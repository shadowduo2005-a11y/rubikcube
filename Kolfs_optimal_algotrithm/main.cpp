#include <iostream>
#include <vector>
#include <string>

#include "Model/RubiksCube3dArray.cpp"
#include "Model/RubiksCube1dArray.cpp"
#include "Model/RubiksCubeBitboard.cpp"
#include "Solver/DFSSolver.h"
#include "Solver/BFSSolver.h"
#include "Solver/IDDFSSolver.h"
#include "Solver/IDAstarSolver.h"
#include "PatternDatabases/CornerDBMaker.h"

using namespace std;

int main() {
    cout << "========================================\n";
    cout << "    Rubik's Cube Solver (IDA* & DB)\n";
    cout << "========================================\n\n";

    string fileName = "Databases/cornerDepth5.txt";

    // Create Corner Database
    cout << "Initializing Corner Pattern Database (may take a few seconds if not built)...\n";
    CornerDBMaker dbMaker(fileName, 0x77);
    dbMaker.bfsAndStore();

    cout << "\nCreating a solved cube and shuffling it...\n";
    RubiksCubeBitboard cube;
    
    // Scramble the cube (14 random moves)
    auto shuffleMoves = cube.randomShuffleCube(14);
    
    cout << "MOVES APPLIED FOR SHUFFLING: ";
    for (auto move: shuffleMoves) cout << cube.getMove(move) << " ";
    cout << "\n\n";
    
    cout << "SHUFFLED CUBE STATE:\n";
    cube.print();

    cout << "\nSolving using IDA* & Corner Pattern Database...\n";
    IDAstarSolver<RubiksCubeBitboard, HashBitboard> idaStarSolver(cube, fileName);
    auto moves = idaStarSolver.solve();

    cout << "MOVES FOR SOLVING: ";
    for (auto move: moves) cout << cube.getMove(move) << " ";
    cout << "\n\n";

    cout << "FINAL CUBE STATE:\n";
    idaStarSolver.rubiksCube.print();
    
    if (idaStarSolver.rubiksCube.isSolved()) {
        cout << "\nStatus: SUCCESSFULLY SOLVED!\n";
    } else {
        cout << "\nStatus: FAILED TO SOLVE!\n";
    }

    return 0;
}