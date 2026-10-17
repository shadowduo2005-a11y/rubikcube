#ifndef CUBELAB_KORF_NON_RECURSIVE_IDA_STAR_H
#define CUBELAB_KORF_NON_RECURSIVE_IDA_STAR_H

#include "../Model/CubieCube.h"
#include "../PatternDatabases/KorfHeuristic.h"

#include <cstdint>
#include <string>
#include <vector>

using namespace std;

struct SearchResult {
    vector<Cube::Move> moves;
    uint64_t expandedNodes = 0;
    uint8_t solutionDepth = 0;
};

class NonRecursiveIDAStar {
public:
    explicit NonRecursiveIDAStar(const string& databaseDirectory)
        : heuristic(databaseDirectory) {}

    SearchResult solve(const Cube& start, uint8_t maximumDepth = 20) const;

    uint8_t estimate(const Cube& cube) const { return heuristic.estimate(cube); }
    const KorfHeuristic& patternDatabases() const { return heuristic; }

private:
    KorfHeuristic heuristic;

    static bool shouldPrune(Cube::Move candidate, int previousFace);
};

#endif
