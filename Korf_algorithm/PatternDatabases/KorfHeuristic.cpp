#include "KorfHeuristic.h"

#include <algorithm>
#include <filesystem>
#include <stdexcept>

using namespace std;

namespace {

string databasePath(const string& directory, const string& fileName) {
    return (filesystem::path(directory) / fileName).string();
}

}  // namespace

KorfHeuristic::KorfHeuristic(const string& databaseDirectory)
    : cornerDatabase(databasePath(databaseDirectory, "corner.pdb"), CORNER_STATES),
      firstEdgeDatabase(databasePath(databaseDirectory, "edgeG1.pdb"), EDGE_PATTERN_STATES),
      secondEdgeDatabase(databasePath(databaseDirectory, "edgeG2.pdb"), EDGE_PATTERN_STATES),
      edgePermutationDatabase(
          databasePath(databaseDirectory, "edge_perm.pdb"), EDGE_PERMUTATION_STATES) {}

uint32_t KorfHeuristic::cornerIndex(const Cube& cube) const {
    array<uint8_t, 8> permutation{};
    array<uint8_t, 7> orientations{};
    for (size_t position = 0; position < permutation.size(); ++position) {
        permutation[position] = cube.corners()[position].index;
        if (position < orientations.size()) {
            orientations[position] = cube.corners()[position].orientation;
        }
    }
    return cornerRanker.rankWithoutReplacement(permutation) * 2187U +
           rankWithReplacement<3>(orientations);
}

uint32_t KorfHeuristic::edgePatternIndex(const Cube& cube, uint8_t firstCubie) const {
    array<uint8_t, 7> positions{};
    array<uint8_t, 7> orientations{};
    size_t found = 0;

    for (uint8_t position = 0; position < cube.edges().size(); ++position) {
        const Cubie edge = cube.edges()[position];
        if (edge.index >= firstCubie && edge.index < firstCubie + 7) {
            const size_t patternPosition = edge.index - firstCubie;
            positions[patternPosition] = position;
            orientations[patternPosition] = edge.orientation;
            ++found;
        }
    }
    if (found != 7) throw logic_error("Cube does not contain the requested edge pattern");

    return edgePatternRanker.rankWithoutReplacement(positions) * 128U +
           rankWithReplacement<2>(orientations);
}

uint32_t KorfHeuristic::firstEdgeIndex(const Cube& cube) const {
    return edgePatternIndex(cube, 0);
}

uint32_t KorfHeuristic::secondEdgeIndex(const Cube& cube) const {
    return edgePatternIndex(cube, 5);
}

uint32_t KorfHeuristic::edgePermutationIndex(const Cube& cube) const {
    array<uint8_t, 12> permutation{};
    for (size_t position = 0; position < permutation.size(); ++position) {
        permutation[position] = cube.edges()[position].index;
    }
    return edgePermutationRanker.rankWithoutReplacement(permutation);
}

uint8_t KorfHeuristic::estimate(const Cube& cube) const {
    const uint8_t corner = cornerDatabase.get(cornerIndex(cube));
    const uint8_t firstEdges = firstEdgeDatabase.get(firstEdgeIndex(cube));
    const uint8_t secondEdges = secondEdgeDatabase.get(secondEdgeIndex(cube));
    const uint8_t edgePermutation =
        edgePermutationDatabase.get(edgePermutationIndex(cube));

    if (corner > 11 || firstEdges > 11 || secondEdges > 11 ||
        edgePermutation > 11) {
        throw runtime_error("A pattern database contains an invalid coordinate");
    }
    return max({corner, firstEdges, secondEdges, edgePermutation});
}
