#include "solver.h"
#include "moves.h"
#include "phases.h"
#include <iostream>

int main() {
    initMoves();
    Cube c = solvedCube();
    c = applyMove(c, moves[4]); // D2
    c = applyMove(c, moves[11]); // R'
    
    std::cout << "Starting solve...\n";
    auto sol = solve(c);
    std::cout << "Done! Moves: " << sol.size() << "\n";
    return 0;
}
