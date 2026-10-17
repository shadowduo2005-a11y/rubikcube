#include "Model/CubieCube.h"
#include "Solver/NonRecursiveIDAStar.h"

#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

namespace {

bool parseScramble(const string& notation, vector<Cube::Move>& moves) {
    istringstream input(notation);
    string token;
    while (input >> token) {
        Cube::Move move{};
        if (!Cube::parseMove(token, move)) return false;
        moves.push_back(move);
    }
    return true;
}

string joinMoves(const vector<Cube::Move>& moves) {
    ostringstream output;
    for (size_t index = 0; index < moves.size(); ++index) {
        if (index != 0) output << ' ';
        output << Cube::moveName(moves[index]);
    }
    return output.str();
}

}  // namespace

int main(int argc, char** argv) {
    const string scramble = argc > 1 ? argv[1] : "";
    const string databaseDirectory = argc > 2 ? argv[2] : "Databases";

    vector<Cube::Move> scrambleMoves;
    if (!parseScramble(scramble, scrambleMoves)) {
        cerr << "Unsupported move notation";
        return 2;
    }

    try {
        Cube scrambledCube;
        for (Cube::Move move : scrambleMoves) scrambledCube.apply(move);

        const auto startedAt = chrono::steady_clock::now();
        NonRecursiveIDAStar solver(databaseDirectory);
        const SearchResult result = solver.solve(scrambledCube);
        const auto finishedAt = chrono::steady_clock::now();

        Cube replay = scrambledCube;
        for (Cube::Move move : result.moves) replay.apply(move);
        if (!replay.isSolved()) {
            cerr << "Korf IDA* returned an unverified result";
            return 3;
        }

        const double elapsedMs =
            chrono::duration<double, milli>(finishedAt - startedAt).count();
        cout << "{\"algorithm\":\"korf\",\"solution\":\""
             << joinMoves(result.moves) << "\",\"moveCount\":"
             << result.moves.size() << ",\"elapsedMs\":" << elapsedMs
             << ",\"expandedNodes\":" << result.expandedNodes
             << ",\"verified\":true}";
        return 0;
    } catch (const exception& error) {
        cerr << error.what();
        return 4;
    }
}
