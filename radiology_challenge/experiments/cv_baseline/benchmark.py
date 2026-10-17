#!/usr/bin/env python3
"""Local, deterministic baselines for the radiology report challenge.

The public competition's exact RES implementation is unavailable.  This module
therefore reports several transparent proxies, led by a field-aware weighted
token edit distance which gives the largest weights to the clinically critical
token classes named in the challenge guide.

No prediction in this file uses a validation target.  Fold-specific retrievers
are fitted only on their training partition.
"""

from __future__ import annotations

import argparse
import json
import math
import re
import time
from collections import Counter, defaultdict
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Callable, Iterable, Sequence

import numpy as np
import pandas as pd
from numba import njit
from rapidfuzz.distance import Levenshtein
from sklearn.feature_extraction.text import TfidfVectorizer
from sklearn.model_selection import GroupKFold, KFold
from sklearn.preprocessing import normalize


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_TRAIN = ROOT / "data" / "train.csv"
DEFAULT_OUT = Path(__file__).resolve().parent / "results"


def norm_text(text: str) -> str:
    return re.sub(r"\s+", " ", str(text).strip().lower())


def lexical_norm(text: str) -> str:
    """Conservative normalization used for retrieval, never for output."""
    text = norm_text(text)
    # High-frequency misspellings in the dictations; deliberately narrow.
    fixes = {
        "degen chnges": "degenerative changes",
        "degen changes": "degenerative changes",
        "norml": "normal",
        "nrmal": "normal",
        "effusuon": "effusion",
        "effusiom": "effusion",
        "osteoarthristis": "osteoarthritis",
        "steartheritis": "osteoarthritis",
        "leukoaraisois": "leukoaraiosis",
        "atheroscelrotsis": "atherosclerosis",
    }
    for old, new in fixes.items():
        text = text.replace(old, new)
    return text


LABEL_RE = re.compile(r"^\s*([A-Z][A-Z0-9 /&()_-]{1,60}):\s*(.*)$")
TOKEN_RE = re.compile(
    r"\d+(?:\.\d+)?(?:[-x×]\d+(?:\.\d+)?){0,2}|"
    r"[a-z]+(?:[-/][a-z]+)*|"
    r"[<>]=?|%",
    re.I,
)

NEGATION = {
    "no", "not", "nor", "without", "absent", "negative", "none", "neither",
}
LATERALITY = {
    "left", "right", "bilateral", "bilaterally", "unilateral", "lt", "rt",
}
SEVERITY = {
    "minimal", "minimally", "mild", "mildly", "moderate", "moderately",
    "marked", "markedly", "severe", "severely", "trace", "small", "large",
    "tiny", "massive", "extensive", "significant",
}
UNITS = {
    "mm", "cm", "m", "ml", "cc", "hz", "mhz", "degrees", "degree",
    "percent", "%", "fr", "french",
}


@dataclass(frozen=True)
class WeightedToken:
    token: str
    field: str
    weight: float


def token_weight(token: str, is_label: bool = False) -> float:
    t = token.lower()
    if is_label:
        return 5.0
    if t in NEGATION or t in LATERALITY or t in UNITS:
        return 6.0
    if t in SEVERITY:
        return 4.0
    if re.fullmatch(r"\d+(?:\.\d+)?(?:[-x×]\d+(?:\.\d+)?){0,2}", t):
        return 6.0
    return 1.0


def field_tokens(text: str) -> list[WeightedToken]:
    """Tokenize text while attaching each token to its report field.

    Labels are explicit tokens. Tokens under the wrong label therefore do not
    match their otherwise-identical reference token, implementing the guide's
    warning that field placement matters.
    """
    result: list[WeightedToken] = []
    section = "unscoped"
    field = "unscoped"
    for raw_line in str(text).replace("\r\n", "\n").replace("\r", "\n").split("\n"):
        match = LABEL_RE.match(raw_line)
        content = raw_line
        if match:
            label = norm_text(match.group(1)).replace(" ", "_")
            if label in {"findings", "impression"}:
                section = label
                field = label
            else:
                field = f"{section}/{label}"
            result.append(WeightedToken(f"__label_{label}__", section, 5.0))
            content = match.group(2)
        for tok in TOKEN_RE.findall(content.lower()):
            result.append(WeightedToken(tok, field, token_weight(tok)))
    return result


@njit(cache=True)
def _weighted_dp(
    ref_ids: np.ndarray,
    ref_weights: np.ndarray,
    hyp_ids: np.ndarray,
    hyp_weights: np.ndarray,
) -> float:
    """Numba kernel: the Python implementation is prohibitively slow for CV."""
    prev = np.zeros(len(hyp_ids) + 1, dtype=np.float64)
    for j in range(1, len(hyp_ids) + 1):
        prev[j] = prev[j - 1] + hyp_weights[j - 1]
    for i in range(len(ref_ids)):
        cur = np.empty(len(hyp_ids) + 1, dtype=np.float64)
        cur[0] = prev[0] + ref_weights[i]
        for j in range(1, len(hyp_ids) + 1):
            if ref_ids[i] == hyp_ids[j - 1]:
                sub_cost = 0.0
            else:
                sub_cost = max(ref_weights[i], hyp_weights[j - 1])
            sub = prev[j - 1] + sub_cost
            delete = prev[j] + ref_weights[i]
            insert = cur[j - 1] + hyp_weights[j - 1]
            cur[j] = min(sub, delete, insert)
        prev = cur
    return prev[-1]


def weighted_edit_distance(reference: str, prediction: str) -> float:
    """Normalized field-aware weighted Levenshtein distance (lower is better)."""
    ref = field_tokens(reference)
    hyp = field_tokens(prediction)
    if not ref:
        return 0.0 if not hyp else 1.0
    # Map the pair's (token, field) identities to compact integers. A lexical
    # match under a different field gets a different ID and thus a penalty.
    ids: dict[tuple[str, str], int] = {}
    for item in (*ref, *hyp):
        ids.setdefault((item.token, item.field), len(ids) + 1)
    ref_ids = np.array([ids[(t.token, t.field)] for t in ref], dtype=np.int64)
    hyp_ids = np.array([ids[(t.token, t.field)] for t in hyp], dtype=np.int64)
    ref_weights = np.array([t.weight for t in ref], dtype=np.float64)
    hyp_weights = np.array([t.weight for t in hyp], dtype=np.float64)
    distance = _weighted_dp(ref_ids, ref_weights, hyp_ids, hyp_weights)
    denom = max(float(ref_weights.sum()), float(hyp_weights.sum()), 1.0)
    return float(distance / denom)


def token_edit_distance(reference: str, prediction: str) -> float:
    ref = [t.lower() for t in TOKEN_RE.findall(reference)]
    hyp = [t.lower() for t in TOKEN_RE.findall(prediction)]
    return float(Levenshtein.normalized_distance(ref, hyp))


def char_edit_distance(reference: str, prediction: str) -> float:
    return float(Levenshtein.normalized_distance(norm_text(reference), norm_text(prediction)))


def critical_tokens(text: str) -> Counter[str]:
    tokens = [t.lower() for t in TOKEN_RE.findall(text)]
    return Counter(
        t for t in tokens
        if token_weight(t) > 1.0
    )


def critical_f1_error(reference: str, prediction: str) -> float:
    a, b = critical_tokens(reference), critical_tokens(prediction)
    overlap = sum((a & b).values())
    denom = sum(a.values()) + sum(b.values())
    return 0.0 if denom == 0 else 1.0 - 2.0 * overlap / denom


def label_f1_error(reference: str, prediction: str) -> float:
    a = Counter(t.token for t in field_tokens(reference) if t.token.startswith("__label_"))
    b = Counter(t.token for t in field_tokens(prediction) if t.token.startswith("__label_"))
    overlap = sum((a & b).values())
    denom = sum(a.values()) + sum(b.values())
    return 0.0 if denom == 0 else 1.0 - 2.0 * overlap / denom


def score_pair(reference: str, prediction: str) -> dict[str, float]:
    return {
        "weighted_field_edit": weighted_edit_distance(reference, prediction),
        "token_edit": token_edit_distance(reference, prediction),
        "char_edit": char_edit_distance(reference, prediction),
        "critical_f1_error": critical_f1_error(reference, prediction),
        "label_f1_error": label_f1_error(reference, prediction),
    }


def split_report(text: str) -> tuple[str, str]:
    parts = re.split(r"(?im)^\s*IMPRESSION:\s*", str(text), maxsplit=1)
    findings = re.sub(r"(?im)^\s*FINDINGS:\s*", "", parts[0], count=1).strip()
    impression = parts[1].strip() if len(parts) > 1 else ""
    return findings, impression


def template_copy(row: pd.Series) -> str:
    return str(row.template_content).strip()


def dictation_findings(row: pd.Series) -> str:
    return f"FINDINGS:\n{str(row.dictation).strip()}\n\nIMPRESSION:\n"


def dictation_both(row: pd.Series) -> str:
    d = str(row.dictation).strip()
    return f"FINDINGS:\n{d}\n\nIMPRESSION:\n{d}"


def dictation_as_report(row: pd.Series) -> str:
    """Preserve existing labels, otherwise put dictated text in FINDINGS."""
    d = str(row.dictation).strip()
    if re.search(r"(?im)^\s*FINDINGS\s*:", d) and re.search(r"(?im)^\s*IMPRESSION\s*:", d):
        # Strip anything outside the requested two sections.
        start = re.search(r"(?im)^\s*FINDINGS\s*:", d).start()
        return d[start:].strip()
    return dictation_findings(row)


def inject_before_impression(row: pd.Series) -> str:
    """Conservative union: template normals plus dictated content in findings."""
    f, i = split_report(str(row.template_content))
    d = str(row.dictation).strip()
    return f"FINDINGS:\n{d}\n\n{f}\n\nIMPRESSION:\n{i}"


def inject_after_findings(row: pd.Series) -> str:
    f, i = split_report(str(row.template_content))
    d = str(row.dictation).strip()
    return f"FINDINGS:\n{f}\n\n{d}\n\nIMPRESSION:\n{i}"


NORMAL_RE = re.compile(
    r"^\s*(?:normal|norml|nrmal|normal study|unremarkable|no (?:acute )?(?:osseous )?abnormalit(?:y|ies)(?: detected)?)[.\s]*$",
    re.I,
)


def normal_aware_template(row: pd.Series) -> str:
    if NORMAL_RE.match(str(row.dictation)):
        return template_copy(row)
    return inject_before_impression(row)


STATIC_METHODS: dict[str, Callable[[pd.Series], str]] = {
    "template_copy": template_copy,
    "dictation_findings": dictation_findings,
    "dictation_both": dictation_both,
    "dictation_as_report": dictation_as_report,
    "inject_before_impression": inject_before_impression,
    "inject_after_findings": inject_after_findings,
    "normal_aware_inject": normal_aware_template,
}


class TfidfRetriever:
    def __init__(
        self,
        analyzer: str = "word",
        template_policy: str = "prefer",
        use_metadata: bool = True,
        transfer: str = "report",
    ) -> None:
        self.analyzer = analyzer
        self.template_policy = template_policy
        self.use_metadata = use_metadata
        self.transfer = transfer

    def _text(self, row: pd.Series) -> str:
        d = lexical_norm(row.dictation)
        if not self.use_metadata:
            return d
        # Prefix categorical values; repetitions provide a modest prior without
        # drowning the dictated findings.
        meta = (
            f"modality_{norm_text(row.modality)} "
            f"body_{norm_text(row.body_part)} "
            f"study_{norm_text(row.study_description)}"
        )
        return f"{meta} {meta} {d}"

    def fit(self, frame: pd.DataFrame) -> "TfidfRetriever":
        self.frame = frame.reset_index(drop=False).rename(columns={"index": "source_index"})
        kwargs = dict(sublinear_tf=True, norm="l2", min_df=1)
        if self.analyzer == "char":
            self.vectorizer = TfidfVectorizer(analyzer="char_wb", ngram_range=(3, 5), max_features=60000, **kwargs)
        else:
            self.vectorizer = TfidfVectorizer(analyzer="word", ngram_range=(1, 2), max_features=40000, **kwargs)
        self.matrix = self.vectorizer.fit_transform([self._text(r) for _, r in self.frame.iterrows()])
        self.templates = np.array([norm_text(x) for x in self.frame.template_content])
        return self

    def _transfer(self, query: pd.Series, source: pd.Series) -> str:
        if self.transfer == "report":
            return str(source.report)
        if self.transfer == "template_if_normal" and NORMAL_RE.match(str(query.dictation)):
            return str(query.template_content)
        if self.transfer == "delta_union":
            # Keep the retrieved report's learned structure and append query
            # dictation to ensure its critical concepts survive. This is a
            # deliberately simple edit-transfer stress test.
            f, i = split_report(str(source.report))
            return f"FINDINGS:\n{str(query.dictation).strip()}\n\n{f}\n\nIMPRESSION:\n{i}"
        raise ValueError(self.transfer)

    def predict_one(self, row: pd.Series) -> tuple[str, int, float]:
        q = self.vectorizer.transform([self._text(row)])
        sims = (self.matrix @ q.T).toarray().ravel()
        same_template = self.templates == norm_text(row.template_content)
        if self.template_policy == "strict" and same_template.any():
            sims = np.where(same_template, sims, -np.inf)
        elif self.template_policy == "prefer":
            sims = sims + 0.18 * same_template.astype(float)
        idx = int(np.argmax(sims))
        source = self.frame.iloc[idx]
        return self._transfer(row, source), int(source.source_index), float(sims[idx])

    def predict(self, frame: pd.DataFrame) -> tuple[list[str], list[int], list[float]]:
        preds, sources, similarities = [], [], []
        for _, row in frame.iterrows():
            p, s, sim = self.predict_one(row)
            preds.append(p)
            sources.append(s)
            similarities.append(sim)
        return preds, sources, similarities


RETRIEVAL_CONFIGS = {
    "nn_word_global": dict(analyzer="word", template_policy="none", use_metadata=True),
    "nn_word_prefer_template": dict(analyzer="word", template_policy="prefer", use_metadata=True),
    "nn_word_strict_template": dict(analyzer="word", template_policy="strict", use_metadata=True),
    "nn_char_prefer_template": dict(analyzer="char", template_policy="prefer", use_metadata=True),
    "nn_char_strict_template": dict(analyzer="char", template_policy="strict", use_metadata=True),
    "nn_dict_only": dict(analyzer="word", template_policy="none", use_metadata=False),
}


def build_duplicate_groups(frame: pd.DataFrame) -> np.ndarray:
    """Connected components linking exact duplicate inputs or targets.

    If two rows share the same normalized (template, dictation) pair OR the same
    normalized report, they are always put in the same fold. This blocks the
    most direct validation leakage without hiding common templates altogether.
    """
    n = len(frame)
    parent = list(range(n))

    def find(x: int) -> int:
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    def union(a: int, b: int) -> None:
        ra, rb = find(a), find(b)
        if ra != rb:
            parent[rb] = ra

    seen: dict[tuple[str, str], int] = {}
    for i, row in frame.reset_index(drop=True).iterrows():
        keys = [
            ("input", norm_text(row.template_content) + "\x1f" + lexical_norm(row.dictation)),
            ("target", norm_text(row.report)),
        ]
        for key in keys:
            if key in seen:
                union(i, seen[key])
            else:
                seen[key] = i
    roots = [find(i) for i in range(n)]
    mapping = {r: j for j, r in enumerate(sorted(set(roots)))}
    return np.array([mapping[r] for r in roots], dtype=int)


def make_splits(frame: pd.DataFrame, scheme: str, folds: int, seed: int):
    if scheme == "random":
        splitter = KFold(n_splits=folds, shuffle=True, random_state=seed)
        return list(splitter.split(frame))
    if scheme == "duplicate_grouped":
        groups = build_duplicate_groups(frame)
    elif scheme == "template_grouped":
        groups = frame.template_content.map(norm_text).to_numpy()
    else:
        raise ValueError(f"Unknown split scheme: {scheme}")
    splitter = GroupKFold(n_splits=folds)
    return list(splitter.split(frame, groups=groups))


def evaluate_predictions(frame: pd.DataFrame, predictions: Sequence[str]) -> tuple[dict, list[dict]]:
    rows = []
    for (_, row), pred in zip(frame.iterrows(), predictions):
        scores = score_pair(str(row.report), str(pred))
        scores.update(
            case_id=row.case_id,
            reference_chars=len(str(row.report)),
            prediction_chars=len(str(pred)),
        )
        rows.append(scores)
    numeric = [k for k in rows[0] if k not in {"case_id"}]
    summary = {}
    for key in numeric:
        vals = np.array([r[key] for r in rows], dtype=float)
        summary[key] = float(vals.mean())
        summary[f"{key}_median"] = float(np.median(vals))
    return summary, rows


def run_benchmark(frame: pd.DataFrame, scheme: str, folds: int, seed: int, out_dir: Path) -> dict:
    split_list = make_splits(frame, scheme, folds, seed)
    all_records: list[dict] = []
    prediction_rows: list[dict] = []
    start = time.time()

    # Static baselines do not fit, but evaluate through the identical fold path
    # so that per-fold uncertainty is visible in the output.
    for fold, (_, val_idx) in enumerate(split_list):
        val = frame.iloc[val_idx]
        for name, fn in STATIC_METHODS.items():
            preds = [fn(row) for _, row in val.iterrows()]
            summary, details = evaluate_predictions(val, preds)
            all_records.append({"method": name, "fold": fold, "n": len(val), **summary})
            for (_, row), pred, detail in zip(val.iterrows(), preds, details):
                prediction_rows.append({
                    "scheme": scheme, "fold": fold, "method": name,
                    "case_id": row.case_id, "prediction": pred,
                    "source_index": None, "similarity": None, **detail,
                })

    for fold, (train_idx, val_idx) in enumerate(split_list):
        train, val = frame.iloc[train_idx], frame.iloc[val_idx]
        for name, config in RETRIEVAL_CONFIGS.items():
            model = TfidfRetriever(**config).fit(train)
            preds, sources, similarities = model.predict(val)
            summary, details = evaluate_predictions(val, preds)
            all_records.append({"method": name, "fold": fold, "n": len(val), **summary})
            for (_, row), pred, source, sim, detail in zip(val.iterrows(), preds, sources, similarities, details):
                prediction_rows.append({
                    "scheme": scheme, "fold": fold, "method": name,
                    "case_id": row.case_id, "prediction": pred,
                    "source_index": source, "similarity": sim, **detail,
                })

    fold_df = pd.DataFrame(all_records)
    pred_df = pd.DataFrame(prediction_rows)
    # Proper case-weighted overall means, plus fold-to-fold SD.
    score_cols = [
        "weighted_field_edit", "token_edit", "char_edit", "critical_f1_error",
        "label_f1_error", "reference_chars", "prediction_chars",
    ]
    overall_rows = []
    for method, group in pred_df.groupby("method"):
        item = {"method": method, "n": len(group)}
        for col in score_cols:
            item[col] = float(group[col].mean())
            folds_for_method = fold_df[fold_df.method == method][col]
            item[f"{col}_fold_sd"] = float(folds_for_method.std(ddof=1))
        overall_rows.append(item)
    overall = pd.DataFrame(overall_rows).sort_values("weighted_field_edit")

    out_dir.mkdir(parents=True, exist_ok=True)
    fold_df.to_csv(out_dir / f"{scheme}_fold_scores.csv", index=False)
    overall.to_csv(out_dir / f"{scheme}_overall.csv", index=False)
    pred_df.to_csv(out_dir / f"{scheme}_predictions.csv", index=False)
    payload = {
        "scheme": scheme, "folds": folds, "seed": seed, "rows": len(frame),
        "elapsed_seconds": time.time() - start,
        "duplicate_groups": int(len(set(build_duplicate_groups(frame)))),
        "template_groups": int(frame.template_content.map(norm_text).nunique()),
        "overall": overall.to_dict(orient="records"),
    }
    (out_dir / f"{scheme}_summary.json").write_text(json.dumps(payload, indent=2))
    return payload


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--train", type=Path, default=DEFAULT_TRAIN)
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT)
    parser.add_argument(
        "--schemes", nargs="+", default=["duplicate_grouped", "template_grouped", "random"],
        choices=["random", "duplicate_grouped", "template_grouped"],
    )
    parser.add_argument("--folds", type=int, default=5)
    parser.add_argument("--seed", type=int, default=20260927)
    args = parser.parse_args()
    frame = pd.read_csv(args.train)
    for scheme in args.schemes:
        result = run_benchmark(frame, scheme, args.folds, args.seed, args.out)
        print(f"\n[{scheme}] {result['elapsed_seconds']:.1f}s")
        display = pd.DataFrame(result["overall"])[
            ["method", "weighted_field_edit", "token_edit", "critical_f1_error", "label_f1_error"]
        ]
        print(display.to_string(index=False, float_format=lambda x: f"{x:.4f}"))


if __name__ == "__main__":
    main()
