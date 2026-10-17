#include "ThistlethwaiteSolver.h"
#include "GroupMoves.h"
#include "../PatternDatabases/PhaseCoordinates.h"

#include <queue>
#include <stdexcept>
#include <vector>

using namespace std;

ThistlethwaiteResult ThistlethwaiteSolver::solve(const Cube& start) const {
    if (!start.isValid()) {
        throw invalid_argument("Cannot search an invalid cube state");
    }

    ThistlethwaiteResult result;
    Cube current = start;

    auto searchPhase = [&](
        const ExactPatternDatabase& db,
        uint32_t (*getCoord)(const Cube&),
        const vector<Cube::Move>& moves,
        size_t phaseIndex) {
        
        struct QueueNode {
            Cube cube;
            vector<Cube::Move> path;
        };

        queue<QueueNode> q;
        q.push({current, {}});

        while (!q.empty()) {
            QueueNode node = q.front();
            q.pop();
            result.expandedNodes++;

            uint8_t currentDist = db.distance(getCoord(node.cube));
            if (currentDist == 0) {
                current = node.cube;
                result.phaseLengths[phaseIndex] = static_cast<uint8_t>(node.path.size());
                result.moves.insert(result.moves.end(), node.path.begin(), node.path.end());
                return;
            }

            for (Cube::Move move : moves) {
                Cube child = node.cube;
                child.apply(move);
                uint8_t childDist = db.distance(getCoord(child));
                if (childDist < currentDist) {
                    vector<Cube::Move> nextPath = node.path;
                    nextPath.push_back(move);
                    q.push({child, move_if_noexcept(nextPath)});
                }
            }
        }
        throw runtime_error("Phase search failed to reach target group");
    };

    searchPhase(patternDbs.group1(), PhaseCoordinates::group1, GroupMoves::group0(), 0);
    searchPhase(patternDbs.group2(), PhaseCoordinates::group2, GroupMoves::group1(), 1);
    searchPhase(patternDbs.group3(), PhaseCoordinates::group3, GroupMoves::group2(), 2);
    searchPhase(patternDbs.group4(), PhaseCoordinates::group4, GroupMoves::group3(), 3);

    return result;
}
