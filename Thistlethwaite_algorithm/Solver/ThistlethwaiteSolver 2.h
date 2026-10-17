#ifndef CUBELAB_THISTLETHWAITE_SOLVER_H
#define CUBELAB_THISTLETHWAITE_SOLVER_H

#include "../Model/CubieCube.h"
#include "../PatternDatabases/PhasePatternDatabases.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

using namespace std;

struct ThistlethwaiteResult {
    vector<Cube::Move> moves;
    array<uint8_t, 4> phaseLengths{};
    uint64_t expandedNodes = 0;
};

class ThistlethwaiteSolver {
public:
    explicit ThistlethwaiteSolver(const string& databaseDirectory)
        : patternDatabases(databaseDirectory) {}

    ThistlethwaiteResult solve(const Cube& start) const;
    const PhasePatternDatabases& databases() const { return patternDatabases; }

private:
    struct Candidate {
        Cube cube;
        Cube::Move move;
        uint8_t distance;
    };

    PhasePatternDatabases patternDatabases;

    const ExactPatternDatabase& databaseForPhase(size_t phase) const;
    uint32_t coordinateForPhase(size_t phase, const Cube& cube) const;
    uint8_t distanceForPhase(size_t phase, const Cube& cube) const;
    bool depthFirst(
        const Cube& cube,
        size_t phase,
        uint8_t remainingDepth,
        int previousFace,
        vector<Cube::Move>& path,
        uint64_t& expandedNodes) const;

    static bool shouldPrune(Cube::Move candidate, int previousFace);
};

#endif
