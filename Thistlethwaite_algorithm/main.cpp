#include "solver.h"
#include "moves.h"
#include <iostream>
#include <cstdlib>
#include <ctime>

int main() {
    initMoves();
    Cube c = solvedCube();

    std::srand(time(nullptr));
    std::cout << "Scramble: ";
    for (int i = 0; i < 15; i++) {
        int m = rand() % moves.size();
        c = applyMove(c, moves[m]);
        std::cout << moveNames[m] << " ";
    }
    std::cout << "\nSolving...\n";

    auto sol = solve(c);
    for (int m : sol)
        std::cout << moveNames[m] << " ";

    std::cout << "\nMoves: " << sol.size() << "\n";
}
