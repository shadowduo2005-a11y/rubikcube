#ifndef CUBELAB_KORF_HEURISTIC_H
#define CUBELAB_KORF_HEURISTIC_H

#include "LehmerRank.h"
#include "MappedNibbleDatabase.h"
#include "../Model/CubieCube.h"

#include <array>
#include <cstdint>
#include <string>

using namespace std;

class KorfHeuristic {
public:
    static constexpr size_t CORNER_STATES = 40320ULL * 2187ULL;
    static constexpr size_t EDGE_PATTERN_STATES = 3991680ULL * 128ULL;
    static constexpr size_t EDGE_PERMUTATION_STATES = 479001600ULL;

    explicit KorfHeuristic(const string& databaseDirectory);

    uint8_t estimate(const Cube& cube) const;

    uint32_t cornerIndex(const Cube& cube) const;
    uint32_t firstEdgeIndex(const Cube& cube) const;
    uint32_t secondEdgeIndex(const Cube& cube) const;
    uint32_t edgePermutationIndex(const Cube& cube) const;

private:
    MappedNibbleDatabase cornerDatabase;
    MappedNibbleDatabase firstEdgeDatabase;
    MappedNibbleDatabase secondEdgeDatabase;
    MappedNibbleDatabase edgePermutationDatabase;

    LehmerRank<8> cornerRanker;
    LehmerRank<12, 7> edgePatternRanker;
    LehmerRank<12> edgePermutationRanker;

    uint32_t edgePatternIndex(const Cube& cube, uint8_t firstCubie) const;
};

#endif
