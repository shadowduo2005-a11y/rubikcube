# 🚀 Rubik's Cube Solver Engine 

A high-performance C++ engine that solves a Rubik's Cube using advanced mathematical models and artificial intelligence search algorithms. The project features two legendary algorithms: **Korf's Optimal Algorithm (IDA* with Pattern Databases)** and **Thistlethwaite’s Algorithm (Group Theory & Subgroup Reductions)**.

---

## 🧠 1. Korf's Optimal Algorithm (IDA* & Corner Pattern Databases)

### Concept Overview
Korf's algorithm finds the shortest possible sequence of moves to solve any Rubik's cube configuration (God's Number = 20). Because a Rubik's cube has $4.3 \times 10^{19}$ possible states, a standard Breadth-First Search (BFS) would run out of memory before depth 7. Korf's algorithm circumvents this by using **Iterative Deepening A* (IDA*)** guided by a pre-computed **Pattern Database**.

### Implementation Details
- **State Representation**: Modeled using three distinct paradigms for comparison: a `1D Array`, `3D Array`, and highly optimized `Bitboards` (using `uint64_t` bit-shifting to represent the 6 faces, yielding massive performance gains).
- **Iterative Deepening A* (IDA*)**: Combines the memory efficiency of Depth-First Search with the optimal shortest-path guarantees of BFS. 
- **Corner Pattern Database (Heuristic)**: We run a backward BFS from the solved state taking *only* the 8 corners into consideration, mapping out exactly how many moves it takes to solve the corners from any state. This data is compressed taking advantage of identical orientation indices, dumped as a serialized binary database.
- **The Engine**: During IDA*, when analyzing a node, we fetch the corner setup from the pattern database. If the database says it takes 8 moves minimum to fix the corners, we can safely prune any branches that have fewer than 8 moves of depth limit remaining. This aggressive pruning breaks the branching factor of the search tree.

### Technical Challenges Faced
- **Database Memory Overhead**: Storing states efficiently without running out of RAM required building a `NibbleArray` (4-bit chunks) because corner move counts rarely exceed 11, vastly reducing the database file footprint to mere megabytes.
- **Bitwise Complexity**: Operating on `RubiksCubeBitboard` required extremely precise and tightly coupled bitwise shifting (`>>`, `<<`, `&`, `|`) to simulate face rotations. A single integer overflow or misaligned bitmask breaks rotational physics.

---

## 🧮 2. Thistlethwaite's Algorithm (Subgroup Reduction)

### Concept Overview
Invented by Morwen Thistlethwaite, this algorithm does not guarantee the *shortest* path, but it guarantees a solution in **at most 45 moves** practically instantly. It works using Group Theory to mathematically restrict the allowed moves progressively through four distinct **phases**.

### Implementation Details
We define the puzzle solving journey across Subgroups:
- **Phase 1 ($G_0 \rightarrow G_1$)**: Restricts moves from `<U, D, L, R, F, B>` to `<U, D, L, R, F2, B2>`. We establish a mathematical goal: **Edge Orientation (EO) must equal 0**. BFS finds the shortest path to this.
- **Phase 2 ($G_1 \rightarrow G_2$)**: Restricts moves to `<U, D, L2, R2, F2, B2>`. The goal is to orient the corners (**CO = 0**) and move the Equatorial (E-slice) edges back into the E-slice. 
- **Phase 3 ($G_2 \rightarrow G_3$)**: Restricts moves to ONLY half-turns `<U2, D2, L2, R2, F2, B2>`. The goal guarantees corners remain in their proper tetrahedral orbits, and fixes Edge Parity.
- **Phase 4 ($G_3 \rightarrow G_{solved}$)**: Using only half-turns, we can quickly snap the cube into its identity solved state because the state space is infinitesimally small ($< 700,000$ states).

### Technical Challenges Faced
- **Graph State Collisions**: Since BFS is done in phases, tracking the full $10^{19}$ mathematical states for a simple `eo=0` check is computationally impossible. I engineered a `hashCubePhase()` function that strips away irrelevant data, hashing *only* the specific slice and orientation structures relevant to the current phase so that the `unordered_set` accurately merges graphs without losing solvable states.
- **Algebraic Symmetries**: Modeling the cycles (`cp` and `ep`) for each face turn required stringent mapping and derivation of 4-cycle disjoint operations. Any misaligned cycle definitions mathematically transformed the model into a non-cube space, failing Phase 2.

---

## 🎯 Interview Preparation / FAQ

**1. Q: Why did you use Iterative Deepening A* instead of regular BFS or Dijkstra?**
> **A:** The branching factor of a Rubik's cube is roughly 13 (18 moves, excluding redundant moves like `L L`). BFS at depth 10 generates $13^{10} \approx 137$ billion states, running out of memory. IDA* only requires $O(\text{depth})$ memory while preserving optimal path findings, which is crucial for exploring depths of 15-20 moves.

**2. Q: What is the purpose of the Corner Pattern Database? Why not use Manhattan Distance?**
> **A:** Manhattan Distance (common in 15-puzzle algorithms) works poorly on the Rubik's cube because a single rotation moves up to 8 pieces simultaneously, heavily overlapping Manhattan distances. A Pattern Database is an exact sub-problem solver (Corners). It's incredibly tight and provides perfect admissibility for the A* heuristic.

**3. Q: How does your Bitboard implementation increase performance?**
> **A:** Traditional 1D/3D matrices require nested loops to move elements individually during a face turn. A 64-bit integer (`uint64_t`) can represent an entire face's 8 edge/corner colors simultaneously. Simulating a spin simply applies bitwise masks (`&`) and shifts (`<<`), acting effectively identically to SIMD vectorizations. CPU cache hits are maximized, causing millions of nodes to be investigated per second.

**4. Q: What was the bug scaling Thistlethwaite's algorithm?**
> **A:** By treating intermediate Phase outputs (like Phase 2) as purely standalone, the graph sometimes transitioned into subgroups that were disjoint from the next Phase's allowed generators. I countered this by tightening mathematical constraints into the node validation logic—guaranteeing that landing conditions precisely respected parity, orientation, and invariant slice groupings.

**5. Q: Which algorithm is better?**
> **A:** It depends on the resource cost. **Korf's algorithm** is the absolute theoretical maximum of efficiency in *move-count*, but scales exponentially in computation time based on the distance. **Thistlethwaite’s Group Theory approach** calculates a path virtually instantly using low memory restrictions, yielding 20-30 moves in milliseconds, which is significantly better for real-time robotic or mobile implementations.
