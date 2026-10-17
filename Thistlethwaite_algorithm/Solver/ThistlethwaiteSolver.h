#ifndef CUBELAB_THISTLETHWAITE_SOLVER_H
#define CUBELAB_THISTLETHWAITE_SOLVER_H

#include "../Model/CubieCube.h"
#include "../PatternDatabases/PhasePatternDatabases.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

struct ThistlethwaiteResult {
    std::vector<Cube::Move> moves;
    uint64_t expandedNodes = 0;
    std::array<uint8_t, 4> phaseLengths = {0, 0, 0, 0};
};

class ThistlethwaiteSolver {
public:
    explicit ThistlethwaiteSolver(const std::string& databaseDirectory)
        : patternDbs(databaseDirectory) {}

    ThistlethwaiteResult solve(const Cube& start) const;
    const PhasePatternDatabases& databases() const { return patternDbs; }

private:
    PhasePatternDatabases patternDbs;
};

#endif
