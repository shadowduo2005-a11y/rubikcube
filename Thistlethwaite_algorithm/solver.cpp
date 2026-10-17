#include "solver.h"
#include "moves.h"
#include "phases.h"
#include <queue>
#include <unordered_set>
#include <vector>
#include <iostream>

struct Node {
    Cube c;
    std::vector<int> path;
};

// Returns allowed moves for the phase
static std::vector<int> getAllowedMoves(int phase) {
    if (phase == 1) return {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17}; // All moves
    if (phase == 2) return {0,1,2,3,4,5,6,7,8,9,10,11,13,16}; // U,D,L,R,F2,B2
    if (phase == 3) return {0,1,2,3,4,5,7,10,13,16}; // U,D,L2,R2,F2,B2
    return {1,4,7,10,13,16}; // Phase 4: U2,D2,L2,R2,F2,B2
}

static size_t hashCubePhase(const Cube& c, int phase) {
    size_t h = 0;
    if (phase == 1) {
        for (int i = 0; i < 12; i++) h = (h << 1) + c.eo[i];
    } else if (phase == 2) {
        for (int i = 0; i < 8; i++) h = (h * 3) + c.co[i];
        for (int i = 8; i < 12; i++) h = (h * 2) + (c.ep[i] >= 8 ? 1 : 0);
    } else if (phase == 3) {
        for (int i = 0; i < 8; i++) h = (h * 8) + c.cp[i];
        for (int i = 0; i < 12; i++) h = (h * 2) + (c.ep[i] >= 8 ? 1 : 0);
    } else {
        // phase 4 is solved cube, hash everything left (ep and cp)
        for (int i = 0; i < 12; i++) h = h * 31 + c.ep[i];
        for (int i = 0; i < 8; i++) h = h * 31 + c.cp[i];
    }
    return h;
}

static std::vector<int> bfs(
    Cube start,
    bool (*goal)(const Cube&),
    int phase
) {
    std::queue<Node> q;
    std::unordered_set<size_t> vis;

    q.push({start,{}});
    vis.insert(hashCubePhase(start, phase));

    auto allowedMoves = getAllowedMoves(phase);

    while (!q.empty()) {
        Node n = q.front(); q.pop();
        if (goal(n.c)) {
            return n.path;
        }

        for (int m : allowedMoves) {
            Cube next = applyMove(n.c, moves[m]);
            size_t h = hashCubePhase(next, phase);
            if (!vis.count(h)) {
                vis.insert(h);
                auto p = n.path;
                p.push_back(m);
                q.push({next,p});
            }
        }
    }
    return {};
}

std::vector<int> solve(Cube c) {
    std::vector<int> sol;
    bool (*goals[])(const Cube&) = {phase1, phase2, phase3, phase4};
    for (int i = 0; i < 4; i++) {
        auto p = bfs(c, goals[i], i + 1);
        for (int m : p) {
            c = applyMove(c, moves[m]);
            sol.push_back(m);
        }
    }
    return sol;
}
