# Exact Korf pattern databases

These are complete reverse-BFS pattern databases in the half-turn metric. Each distance occupies four bits; even coordinates use the high nibble and odd coordinates use the low nibble.

| File | Abstract states | Maximum | Bytes | Meaning |
|---|---:|---:|---:|---|
| `corner.pdb` | 88,179,840 | 11 | 44,089,921 | permutation and orientation of all 8 corners |
| `edgeG1.pdb` | 510,935,040 | 11 | 255,467,521 | position and orientation of edge cubies 0–6 |
| `edgeG2.pdb` | 510,935,040 | 11 | 255,467,521 | position and orientation of edge cubies 5–11 |
| `edge_perm.pdb` | 479,001,600 | 10 | 239,500,801 | permutation of all 12 edges |

The 7-edge tables overlap on cubies 5 and 6; the solver therefore combines all four estimates using `max`, never addition. This preserves admissibility.

The files are the published precomputed artifacts from [benbotto/rubiks-cube-cracker](https://github.com/benbotto/rubiks-cube-cracker), whose documented Korf extension uses these exact coordinates. `SHA256SUMS` records the verified Git LFS object identities. The final padding byte in each file is retained for format compatibility and is never indexed.
