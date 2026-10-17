"use strict";

const MOVE_PATTERN = /^[URFDLB](?:2|')?$/;
const DEFAULT_SCRAMBLE = "R U R' U' F2 L";
const ALGORITHMS = {
  thistlethwaite: {
    id: "thistlethwaite",
    name: "Thistlethwaite",
    shortName: "Four-phase",
    description: "Four restricted IDDFS phases guided by locally generated exact databases.",
    randomLength: 10,
  },
  korf: {
    id: "korf",
    name: "Korf IDA*",
    shortName: "Optimal search",
    description: "Optimal non-recursive IDA* guided by four exact pattern databases.",
    randomLength: 5,
  },
};

const FACE_DEFINITIONS = {
  U: { normal: [0, 1, 0], row: [0, 0, 1], column: [1, 0, 0] },
  R: { normal: [1, 0, 0], row: [0, -1, 0], column: [0, 0, -1] },
  F: { normal: [0, 0, 1], row: [0, -1, 0], column: [1, 0, 0] },
  D: { normal: [0, -1, 0], row: [0, 0, -1], column: [1, 0, 0] },
  L: { normal: [-1, 0, 0], row: [0, -1, 0], column: [0, 0, 1] },
  B: { normal: [0, 0, -1], row: [0, -1, 0], column: [-1, 0, 0] },
};

function addVector(...vectors) {
  return vectors.reduce(
    (sum, vector) => [sum[0] + vector[0], sum[1] + vector[1], sum[2] + vector[2]],
    [0, 0, 0],
  );
}

function scaleVector(vector, amount) {
  return vector.map((value) => value * amount);
}

function vectorKey(normal, position) {
  return `${normal.join(",")}|${position.join(",")}`;
}

function rotateVector(vector, axis, direction) {
  const [x, y, z] = vector;
  if (axis === 0) return direction > 0 ? [x, -z, y] : [x, z, -y];
  if (axis === 1) return direction > 0 ? [z, y, -x] : [-z, y, x];
  return direction > 0 ? [-y, x, z] : [y, -x, z];
}

function parseAlgorithm(value) {
  const prepared = String(value)
    .replace(/[’`]/g, "'")
    .replace(/,/g, " ")
    .trim()
    .toUpperCase();
  if (!prepared) return [];

  const moves = prepared.split(/\s+/);
  if (moves.length > 100) throw new Error("Keep the algorithm under 100 face turns.");
  const invalid = moves.find((move) => !MOVE_PATTERN.test(move));
  if (invalid) {
    throw new Error(`“${invalid}” is not supported. Use U, R, F, D, L, B with 2 or '.`);
  }
  return moves;
}

function normalizeAlgorithm(value) {
  return parseAlgorithm(value).join(" ");
}

class CubeModel {
  constructor(stickers = null) {
    this.stickers = stickers || CubeModel.createSolvedStickers();
  }

  static createSolvedStickers() {
    const stickers = [];
    for (const [face, definition] of Object.entries(FACE_DEFINITIONS)) {
      for (let row = 0; row < 3; row += 1) {
        for (let column = 0; column < 3; column += 1) {
          stickers.push({
            color: face,
            normal: [...definition.normal],
            position: addVector(
              definition.normal,
              scaleVector(definition.row, row - 1),
              scaleVector(definition.column, column - 1),
            ),
          });
        }
      }
    }
    return stickers;
  }

  clone() {
    return new CubeModel(
      this.stickers.map((sticker) => ({
        color: sticker.color,
        normal: [...sticker.normal],
        position: [...sticker.position],
      })),
    );
  }

  applyMove(move) {
    const face = move[0];
    const definition = FACE_DEFINITIONS[face];
    const axis = definition.normal.findIndex((value) => value !== 0);
    const layer = definition.normal[axis];
    const clockwiseDirection = -layer;
    const turns = move.endsWith("2") ? 2 : move.endsWith("'") ? 3 : 1;

    for (let turn = 0; turn < turns; turn += 1) {
      for (const sticker of this.stickers) {
        if (sticker.position[axis] !== layer) continue;
        sticker.position = rotateVector(sticker.position, axis, clockwiseDirection);
        sticker.normal = rotateVector(sticker.normal, axis, clockwiseDirection);
      }
    }
    return this;
  }

  applyAlgorithm(algorithm) {
    parseAlgorithm(algorithm).forEach((move) => this.applyMove(move));
    return this;
  }

  faceColors(face) {
    const definition = FACE_DEFINITIONS[face];
    const lookup = new Map(
      this.stickers.map((sticker) => [vectorKey(sticker.normal, sticker.position), sticker.color]),
    );
    const colors = [];
    for (let row = 0; row < 3; row += 1) {
      for (let column = 0; column < 3; column += 1) {
        const position = addVector(
          definition.normal,
          scaleVector(definition.row, row - 1),
          scaleVector(definition.column, column - 1),
        );
        colors.push(lookup.get(vectorKey(definition.normal, position)));
      }
    }
    return colors;
  }

  isSolved() {
    return Object.keys(FACE_DEFINITIONS).every((face) =>
      this.faceColors(face).every((color) => color === face),
    );
  }
}

function randomInteger(limit) {
  const value = new Uint32Array(1);
  crypto.getRandomValues(value);
  return value[0] % limit;
}

function generateRandomScramble(algorithm) {
  const configuration = ALGORITHMS[algorithm];
  if (!configuration) throw new Error("Choose a supported solver.");
  const faces = ["U", "R", "F", "D", "L", "B"];
  const axes = [0, 1, 2, 0, 1, 2];
  const suffixes = ["", "'", "2"];
  const moves = [];
  let previousAxis = -1;

  while (moves.length < configuration.randomLength) {
    const faceIndex = randomInteger(faces.length);
    if (axes[faceIndex] === previousAxis) continue;
    previousAxis = axes[faceIndex];
    moves.push(`${faces[faceIndex]}${suffixes[randomInteger(suffixes.length)]}`);
  }
  return moves.join(" ");
}

async function requestSolution(scramble, algorithm) {
  if (!ALGORITHMS[algorithm]) throw new Error("Choose a supported solver.");
  const response = await fetch("/api/solve", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ algorithm, scramble }),
  });
  const result = await response.json();
  if (!response.ok) throw new Error(result.error || "The native solver could not finish.");
  if (result.algorithm !== algorithm) throw new Error("The wrong native algorithm returned a result.");

  const solution = normalizeAlgorithm(result.solution || "");
  const verificationCube = new CubeModel().applyAlgorithm(scramble).applyAlgorithm(solution);
  if (!result.verified || !verificationCube.isSolved()) {
    throw new Error("The native result failed independent JavaScript replay.");
  }

  return {
    algorithm,
    scramble,
    solution,
    moveCount: parseAlgorithm(solution).length,
    elapsedMs: Number(result.elapsedMs) || 0,
  };
}

const elements = {
  algorithmButtons: [...document.querySelectorAll("[data-algorithm]")],
  algorithmNote: document.querySelector("#algorithm-note"),
  clearButton: document.querySelector("#clear-button"),
  endPlayer: document.querySelector("#end-player"),
  engineLabel: document.querySelector("#engine-label"),
  engineStatus: document.querySelector("#engine-status"),
  faceElements: {},
  messageIcon: document.querySelector("#message-icon"),
  messageText: document.querySelector("#message-text"),
  metricAlgorithm: document.querySelector("#metric-algorithm"),
  metricMoves: document.querySelector("#metric-moves"),
  metricReplay: document.querySelector("#metric-replay"),
  metricTime: document.querySelector("#metric-time"),
  moveButtons: [...document.querySelectorAll("[data-move]")],
  playSolution: document.querySelector("#play-solution"),
  randomButton: document.querySelector("#random-button"),
  resetPlayer: document.querySelector("#reset-player"),
  scramble: document.querySelector("#scramble"),
  solutionOutput: document.querySelector("#solution-output"),
  solutionPanel: document.querySelector("#solution-panel"),
  solveButton: document.querySelector("#solve-button"),
  solveLabel: document.querySelector("#solve-label"),
};

const statusLabels = {
  idle: "Engine ready",
  generating: "Generating",
  solving: "Search running",
  solved: "Verified solved",
  error: "Input issue",
};

const state = {
  algorithm: "thistlethwaite",
  appliedScramble: DEFAULT_SCRAMBLE,
  copied: false,
  elapsedMs: 0,
  message: "Ready for a legal 3×3 scramble.",
  moveCount: 0,
  playbackIndex: 0,
  playing: false,
  solution: "",
  solvedAlgorithm: "",
  status: "idle",
};

let playbackToken = 0;

function buildCubeNet() {
  document.querySelectorAll("[data-face]").forEach((faceElement) => {
    const face = faceElement.dataset.face;
    const label = document.createElement("span");
    label.className = "face-name";
    label.textContent = face;
    const grid = document.createElement("div");
    grid.className = "face-grid";
    elements.faceElements[face] = [];
    for (let index = 0; index < 9; index += 1) {
      const sticker = document.createElement("span");
      sticker.className = "sticker";
      sticker.setAttribute("aria-hidden", "true");
      grid.append(sticker);
      elements.faceElements[face].push(sticker);
    }
    faceElement.append(label, grid);
  });
}

function isBusy() {
  return state.status === "solving" || state.status === "generating";
}

function stopPlayback() {
  state.playing = false;
  playbackToken += 1;
}

function resetResult() {
  stopPlayback();
  state.solution = "";
  state.solvedAlgorithm = "";
  state.moveCount = 0;
  state.elapsedMs = 0;
  state.playbackIndex = 0;
}

function setStatus(status, message) {
  state.status = status;
  state.message = message;
  render();
}

function formatElapsed(milliseconds) {
  return milliseconds < 1000
    ? `${Math.round(milliseconds)} ms`
    : `${(milliseconds / 1000).toFixed(2)} s`;
}

function cubeAtPlaybackPosition() {
  const cube = new CubeModel().applyAlgorithm(state.appliedScramble);
  parseAlgorithm(state.solution)
    .slice(0, state.playbackIndex)
    .forEach((move) => cube.applyMove(move));
  return cube;
}

function renderCube() {
  const cube = cubeAtPlaybackPosition();
  for (const face of Object.keys(FACE_DEFINITIONS)) {
    cube.faceColors(face).forEach((color, index) => {
      elements.faceElements[face][index].className = `sticker sticker-${color.toLowerCase()}`;
    });
  }
}

function renderSolution() {
  elements.solutionOutput.replaceChildren();
  if (!state.solution) {
    const paragraph = document.createElement("p");
    paragraph.textContent =
      state.status === "solved"
        ? "The cube was already solved; no turns are required."
        : "Run the solver to generate a move-by-move verified sequence.";
    elements.solutionOutput.append(paragraph);
    return;
  }

  const sequence = document.createElement("div");
  sequence.className = "move-sequence";
  sequence.setAttribute("aria-label", "Solution sequence");
  parseAlgorithm(state.solution).forEach((move, index) => {
    const item = document.createElement("span");
    if (index < state.playbackIndex) item.classList.add("is-played");
    const number = document.createElement("small");
    number.textContent = String(index + 1).padStart(2, "0");
    item.append(number, document.createTextNode(move.replace("'", "′")));
    sequence.append(item);
  });

  const copyButton = document.createElement("button");
  copyButton.type = "button";
  copyButton.className = "copy-button";
  copyButton.textContent = state.copied ? "Copied ✓" : "Copy sequence";
  copyButton.addEventListener("click", copySolution);
  elements.solutionOutput.append(sequence, copyButton);
}

function render() {
  const busy = isBusy();
  const selected = ALGORITHMS[state.algorithm];
  elements.algorithmButtons.forEach((button) => {
    const isSelected = button.dataset.algorithm === state.algorithm;
    button.classList.toggle("is-selected", isSelected);
    button.setAttribute("aria-checked", String(isSelected));
    button.disabled = busy;
  });
  elements.algorithmNote.textContent = selected.description;
  elements.solveLabel.textContent =
    state.status === "solving" ? "Solving…" : `Solve with ${selected.name}`;
  elements.randomButton.textContent = state.status === "generating" ? "Generating…" : "New scramble";

  elements.solveButton.disabled = busy;
  elements.randomButton.disabled = busy;
  elements.clearButton.disabled = busy;
  elements.scramble.disabled = busy;
  elements.moveButtons.forEach((button) => {
    button.disabled = busy;
  });

  elements.engineStatus.className = `engine-status status-${state.status}`;
  elements.engineLabel.textContent = statusLabels[state.status];
  elements.messageIcon.className = `message-icon message-${state.status}`;
  elements.messageIcon.textContent = state.status === "error" ? "!" : state.status === "solved" ? "✓" : "i";
  elements.messageText.textContent = state.message;

  const solved = state.status === "solved";
  elements.solutionPanel.classList.toggle("has-solution", solved);
  elements.metricMoves.textContent = solved ? String(state.moveCount) : "—";
  elements.metricTime.textContent = solved ? formatElapsed(state.elapsedMs) : "—";
  elements.metricReplay.textContent = solved ? "PASS" : "—";
  elements.metricAlgorithm.textContent =
    solved && state.solvedAlgorithm ? ALGORITHMS[state.solvedAlgorithm].shortName : "—";

  const hasSolution = Boolean(state.solution);
  elements.resetPlayer.disabled = false;
  elements.playSolution.disabled = !hasSolution;
  elements.playSolution.textContent = state.playing ? "Ⅱ Pause" : "▶ Play solution";
  elements.endPlayer.disabled = !hasSolution;
  renderSolution();
  renderCube();
}

async function handleSolve() {
  resetResult();
  try {
    const requestedAlgorithm = state.algorithm;
    const normalized = normalizeAlgorithm(elements.scramble.value);
    elements.scramble.value = normalized;
    state.appliedScramble = normalized;
    setStatus("solving", `Running ${ALGORITHMS[requestedAlgorithm].name} in C++ and replaying the result…`);
    const result = await requestSolution(normalized, requestedAlgorithm);
    elements.scramble.value = result.scramble;
    state.appliedScramble = result.scramble;
    state.solution = result.solution;
    state.solvedAlgorithm = result.algorithm;
    state.moveCount = result.moveCount;
    state.elapsedMs = result.elapsedMs;
    state.playbackIndex = 0;
    setStatus(
      "solved",
      result.moveCount === 0 ? "This state is already solved." : "Solution verified by replaying every move.",
    );
  } catch (error) {
    setStatus("error", error instanceof Error ? error.message : "Unable to solve this input.");
  }
}

function handleRandomize() {
  try {
    setStatus("generating", `Generating a ${ALGORITHMS[state.algorithm].name} demo scramble…`);
    const scramble = generateRandomScramble(state.algorithm);
    elements.scramble.value = scramble;
    state.appliedScramble = scramble;
    resetResult();
    setStatus("idle", "Scramble loaded. Press solve when you’re ready.");
  } catch {
    setStatus("error", "The scramble generator could not start. Try again.");
  }
}

function appendMove(move) {
  try {
    const normalized = normalizeAlgorithm(`${elements.scramble.value.trim()} ${move}`.trim());
    elements.scramble.value = normalized;
    state.appliedScramble = normalized;
    resetResult();
    setStatus("idle", `${move} applied to the 2D cube.`);
  } catch (error) {
    setStatus("error", error instanceof Error ? error.message : "Move could not be applied.");
  }
}

function clearCube() {
  elements.scramble.value = "";
  state.appliedScramble = "";
  resetResult();
  setStatus("idle", "Cube reset to the solved state.");
}

function previewInput() {
  if (isBusy()) return;
  resetResult();
  try {
    const normalized = normalizeAlgorithm(elements.scramble.value);
    elements.scramble.value = normalized;
    state.appliedScramble = normalized;
    setStatus("idle", `${parseAlgorithm(normalized).length} legal turns loaded.`);
  } catch (error) {
    setStatus("error", error instanceof Error ? error.message : "Invalid scramble.");
  }
}

function chooseAlgorithm(algorithm) {
  if (isBusy() || !ALGORITHMS[algorithm]) return;
  state.algorithm = algorithm;
  resetResult();
  setStatus(
    "idle",
    algorithm === "korf"
      ? "Korf IDA* selected. Short scrambles are recommended for optimal search."
      : "Thistlethwaite exact-PDB IDDFS selected for fast four-group solving.",
  );
}

function resetPlayback() {
  stopPlayback();
  state.playbackIndex = 0;
  render();
}

function finishPlayback() {
  stopPlayback();
  state.playbackIndex = parseAlgorithm(state.solution).length;
  render();
}

async function togglePlayback() {
  if (state.playing) {
    stopPlayback();
    render();
    return;
  }
  const moves = parseAlgorithm(state.solution);
  if (!moves.length) return;
  if (state.playbackIndex >= moves.length) state.playbackIndex = 0;
  state.playing = true;
  const token = ++playbackToken;
  render();

  while (state.playbackIndex < moves.length && state.playing && token === playbackToken) {
    await new Promise((resolve) => window.setTimeout(resolve, 430));
    if (!state.playing || token !== playbackToken) return;
    state.playbackIndex += 1;
    render();
  }
  if (token === playbackToken) {
    state.playing = false;
    render();
  }
}

async function copySolution() {
  if (!state.solution) return;
  await navigator.clipboard.writeText(state.solution);
  state.copied = true;
  render();
  window.setTimeout(() => {
    state.copied = false;
    render();
  }, 1600);
}

elements.algorithmButtons.forEach((button) => {
  button.addEventListener("click", () => chooseAlgorithm(button.dataset.algorithm));
});
elements.clearButton.addEventListener("click", clearCube);
elements.endPlayer.addEventListener("click", finishPlayback);
elements.moveButtons.forEach((button) => {
  button.addEventListener("click", () => appendMove(button.dataset.move));
});
elements.playSolution.addEventListener("click", togglePlayback);
elements.randomButton.addEventListener("click", handleRandomize);
elements.resetPlayer.addEventListener("click", resetPlayback);
elements.scramble.addEventListener("blur", previewInput);
elements.solveButton.addEventListener("click", handleSolve);

window.CubeLab = { CubeModel, normalizeAlgorithm, parseAlgorithm };
buildCubeNet();
render();
