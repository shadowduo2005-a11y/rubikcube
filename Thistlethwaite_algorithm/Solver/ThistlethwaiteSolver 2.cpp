#include "ThistlethwaiteSolver.h"
#include "GroupMoves.h"
#include "../PatternDatabases/PhaseCoordinates.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <vector>

using namespace std;

namespace {

const vector<Cube::Move>& movesForPhase(size_t phase) {
    switch (phase) {
        case 0: return GroupMoves::group0();
        case 1: return GroupMoves::group1();
        case 2: return GroupMoves::group2();
        case 3: return GroupMoves::group3();
        default: throw out_of_range("Thistlethwaite phase is out of range");
    }
}

}  // namespace

const ExactPatternDatabase& ThistlethwaiteSolver::databaseForPhase(size_t phase) const {
    switch (phase) {
        case 0: return patternDatabases.group1();
        case 1: return patternDatabases.group2();
        case 2: return patternDatabases.group3();
        case 3: return patternDatabases.group4();
        default: throw out_of_range("Thistlethwaite phase is out of range");
    }
}

uint32_t ThistlethwaiteSolver::coordinateForPhase(size_t phase, const Cube& cube) const {
    switch (phase) {
        case 0: return PhaseCoordinates::group1(cube);
        case 1: return PhaseCoordinates::group2(cube);
        case 2: return PhaseCoordinates::group3(cube);
        case 3: return PhaseCoordinates::group4(cube);
        default: throw out_of_range("Thistlethwaite phase is out of range");
    }
}

uint8_t ThistlethwaiteSolver::distanceForPhase(size_t phase, const Cube& cube) const {
    return databaseForPhase(phase).distance(coordinateForPhase(phase, cube));
}

bool ThistlethwaiteSolver::shouldPrune(Cube::Move candidate, int previousFace) {
    if (previousFace < 0) return false;
    const int candidateFace = Cube::face(candidate);
    if (candidateFace == previousFace) return true;

    static constexpr array<int, 6> OPPOSITE_FACE = {1, 0, 3, 2, 5, 4};
    return candidateFace == OPPOSITE_FACE[previousFace] &&
           candidateFace < previousFace;
}

bool ThistlethwaiteSolver::depthFirst(
    const Cube& cube,
    size_t phase,
    uint8_t remainingDepth,
    int previousFace,
    vector<Cube::Move>& path,
    uint64_t& expandedNodes) const {
    const uint8_t currentDistance = distanceForPhase(phase, cube);
    if (currentDistance == 0) return true;
    if (currentDistance > remainingDepth || remainingDepth == 0) return false;

    ++expandedNodes;
    vector<Candidate> candidates;
    candidates.reserve(movesForPhase(phase).size());
    for (Cube::Move move : movesForPhase(phase)) {
        if (shouldPrune(move, previousFace)) continue;
        Cube child = cube;
        child.apply(move);
        const uint8_t childDistance = distanceForPhase(phase, child);
        if (childDistance <= remainingDepth - 1) {
            candidates.push_back({std::move(child), move, childDistance});
        }
    }
    stable_sort(
        candidates.begin(), candidates.end(),
        [](const Candidate& left, const Candidate& right) {
            if (left.distance != right.distance) return left.distance < right.distance;
            return static_cast<int>(left.move) < static_cast<int>(right.move);
        });

    for (const Candidate& candidate : candidates) {
        path.push_back(candidate.move);
        if (depthFirst(
                candidate.cube, phase, static_cast<uint8_t>(remainingDepth - 1),
                Cube::face(candidate.move), path, expandedNodes)) {
            return true;
        }
        path.pop_back();
    }
    return false;
}

ThistlethwaiteResult ThistlethwaiteSolver::solve(const Cube& start) const {
    if (!start.isValid()) throw invalid_argument("Cannot solve an invalid cube state");

    Cube cube = start;
    ThistlethwaiteResult result;
    for (size_t phase = 0; phase < 4; ++phase) {
        vector<Cube::Move> phasePath;
        const uint8_t lowerBound = distanceForPhase(phase, cube);
        bool found = false;

        // Exact PDB values are the first viable IDDFS depth. The loop remains
        // an iterative-deepening search and guards against database/search bugs.
        for (uint8_t depthLimit = lowerBound;
             depthLimit <= databaseForPhase(phase).maximumDistance();
             ++depthLimit) {
            phasePath.clear();
            if (depthFirst(
                    cube, phase, depthLimit, -1, phasePath,
                    result.expandedNodes)) {
                found = true;
                break;
            }
        }
        if (!found) {
            throw runtime_error(
                "IDDFS could not complete Thistlethwaite phase " +
                to_string(phase + 1));
        }

        for (Cube::Move move : phasePath) cube.apply(move);
        result.phaseLengths[phase] = static_cast<uint8_t>(phasePath.size());
        result.moves.insert(result.moves.end(), phasePath.begin(), phasePath.end());

        const bool phaseVerified =
            phase == 0 ? PhaseCoordinates::inGroup1(cube) :
            phase == 1 ? PhaseCoordinates::inGroup2(cube) :
            phase == 2 ? distanceForPhase(phase, cube) == 0 :
                         cube.isSolved();
        if (!phaseVerified) {
            throw runtime_error(
                "Thistlethwaite phase " + to_string(phase + 1) +
                " failed native group verification");
        }
    }

    if (!cube.isSolved()) throw runtime_error("Thistlethwaite final replay is not solved");
    return result;
}
