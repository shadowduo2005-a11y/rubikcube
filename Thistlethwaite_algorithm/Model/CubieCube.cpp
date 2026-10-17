#include "CubieCube.h"

#include <array>

using namespace std;

namespace {

struct MoveTable {
    array<uint8_t, 8> cornerSource;
    array<uint8_t, 8> cornerTwist;
    array<uint8_t, 12> edgeSource;
    array<uint8_t, 12> edgeFlip;
};

// Destination-to-source permutations for standard clockwise U,D,L,R,F,B.
// F and B flip their four edges in this orientation convention.
const array<MoveTable, 6> MOVE_TABLES = {{
    {{{3, 0, 1, 2, 4, 5, 6, 7}},
     {{0, 0, 0, 0, 0, 0, 0, 0}},
     {{3, 0, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11}},
     {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}},
    {{{0, 1, 2, 3, 5, 6, 7, 4}},
     {{0, 0, 0, 0, 0, 0, 0, 0}},
     {{0, 1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 8}},
     {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}},
    {{{5, 1, 2, 0, 3, 4, 6, 7}},
     {{2, 0, 0, 1, 2, 1, 0, 0}},
     {{0, 1, 2, 6, 4, 3, 9, 7, 8, 5, 10, 11}},
     {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}},
    {{{0, 2, 7, 3, 4, 5, 1, 6}},
     {{0, 1, 2, 0, 0, 0, 2, 1}},
     {{0, 4, 2, 3, 11, 5, 6, 1, 8, 9, 10, 7}},
     {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}},
    {{{0, 1, 3, 4, 7, 5, 6, 2}},
     {{0, 0, 1, 2, 1, 0, 0, 2}},
     {{0, 1, 5, 3, 2, 8, 6, 7, 4, 9, 10, 11}},
     {{0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 0, 0}}},
    {{{1, 6, 2, 3, 4, 0, 5, 7}},
     {{1, 2, 0, 0, 0, 2, 1, 0}},
     {{7, 1, 2, 3, 4, 5, 0, 10, 8, 9, 6, 11}},
     {{1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0}}},
}};

const array<string, 18> MOVE_NAMES = {
    "U", "U'", "U2", "D", "D'", "D2",
    "L", "L'", "L2", "R", "R'", "R2",
    "F", "F'", "F2", "B", "B'", "B2",
};

int permutationParity(const uint8_t* permutation, size_t size) {
    int inversions = 0;
    for (size_t left = 0; left < size; ++left) {
        for (size_t right = left + 1; right < size; ++right) {
            inversions += permutation[left] > permutation[right];
        }
    }
    return inversions & 1;
}

}  // namespace

Cube::Cube() {
    for (uint8_t position = 0; position < cornerCubies.size(); ++position) {
        cornerCubies[position] = {position, 0};
    }
    for (uint8_t position = 0; position < edgeCubies.size(); ++position) {
        edgeCubies[position] = {position, 0};
    }
}

void Cube::applyQuarterTurn(int faceIndex) {
    const MoveTable& table = MOVE_TABLES.at(static_cast<size_t>(faceIndex));
    const auto oldCorners = cornerCubies;
    const auto oldEdges = edgeCubies;

    for (size_t position = 0; position < cornerCubies.size(); ++position) {
        const Cubie source = oldCorners[table.cornerSource[position]];
        cornerCubies[position] = {
            source.index,
            static_cast<uint8_t>((source.orientation + table.cornerTwist[position]) % 3),
        };
    }
    for (size_t position = 0; position < edgeCubies.size(); ++position) {
        const Cubie source = oldEdges[table.edgeSource[position]];
        edgeCubies[position] = {
            source.index,
            static_cast<uint8_t>(source.orientation ^ table.edgeFlip[position]),
        };
    }
}

Cube& Cube::apply(Move move) {
    const int rawMove = static_cast<int>(move);
    const int turnKind = rawMove % 3;
    const int quarterTurns = turnKind == 0 ? 1 : turnKind == 1 ? 3 : 2;
    for (int turn = 0; turn < quarterTurns; ++turn) {
        applyQuarterTurn(rawMove / 3);
    }
    return *this;
}

bool Cube::isSolved() const {
    for (uint8_t position = 0; position < cornerCubies.size(); ++position) {
        if (cornerCubies[position].index != position ||
            cornerCubies[position].orientation != 0) {
            return false;
        }
    }
    for (uint8_t position = 0; position < edgeCubies.size(); ++position) {
        if (edgeCubies[position].index != position || edgeCubies[position].orientation != 0) {
            return false;
        }
    }
    return true;
}

bool Cube::isValid() const {
    array<bool, 8> seenCorners{};
    array<bool, 12> seenEdges{};
    array<uint8_t, 8> cornerPermutation{};
    array<uint8_t, 12> edgePermutation{};
    int cornerOrientationSum = 0;
    int edgeOrientationSum = 0;

    for (size_t position = 0; position < cornerCubies.size(); ++position) {
        const Cubie cubie = cornerCubies[position];
        if (cubie.index >= 8 || cubie.orientation >= 3 || seenCorners[cubie.index]) {
            return false;
        }
        seenCorners[cubie.index] = true;
        cornerPermutation[position] = cubie.index;
        cornerOrientationSum += cubie.orientation;
    }
    for (size_t position = 0; position < edgeCubies.size(); ++position) {
        const Cubie cubie = edgeCubies[position];
        if (cubie.index >= 12 || cubie.orientation >= 2 || seenEdges[cubie.index]) {
            return false;
        }
        seenEdges[cubie.index] = true;
        edgePermutation[position] = cubie.index;
        edgeOrientationSum += cubie.orientation;
    }

    return cornerOrientationSum % 3 == 0 && edgeOrientationSum % 2 == 0 &&
           permutationParity(cornerPermutation.data(), cornerPermutation.size()) ==
               permutationParity(edgePermutation.data(), edgePermutation.size());
}

Cube::Move Cube::inverse(Move move) {
    const int rawMove = static_cast<int>(move);
    const int turnKind = rawMove % 3;
    if (turnKind == 0) return static_cast<Move>(rawMove + 1);
    if (turnKind == 1) return static_cast<Move>(rawMove - 1);
    return move;
}

string Cube::moveName(Move move) {
    return MOVE_NAMES.at(static_cast<size_t>(move));
}

bool Cube::parseMove(const string& token, Move& move) {
    for (size_t index = 0; index < MOVE_NAMES.size(); ++index) {
        if (MOVE_NAMES[index] == token) {
            move = static_cast<Move>(index);
            return true;
        }
    }
    return false;
}
