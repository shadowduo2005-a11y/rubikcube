#include "NonRecursiveIDAStar.h"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace std;

namespace {

struct ChildNode {
    Cube cube;
    Cube::Move move;
    uint8_t heuristic;
};

struct SearchFrame {
    Cube cube;
    uint8_t depth;
    int previousFace;
    bool expanded = false;
    size_t nextChild = 0;
    vector<ChildNode> children;
};

}  // namespace

bool NonRecursiveIDAStar::shouldPrune(Cube::Move candidate, int previousFace) {
    if (previousFace < 0) return false;

    const int candidateFace = Cube::face(candidate);
    if (candidateFace == previousFace) {
        // Covers F F, F F', F F2, and the equivalent cases on every face.
        return true;
    }

    static constexpr array<int, 6> OPPOSITE_FACE = {1, 0, 3, 2, 5, 4};
    // Opposite faces commute. Keep one canonical adjacent ordering.
    return candidateFace == OPPOSITE_FACE[previousFace] &&
           candidateFace < previousFace;
}

SearchResult NonRecursiveIDAStar::solve(const Cube& start, uint8_t maximumDepth) const {
    if (!start.isValid()) throw invalid_argument("Cannot search an invalid cube state");

    SearchResult result;
    if (start.isSolved()) return result;

    int bound = heuristic.estimate(start);
    while (bound <= maximumDepth) {
        int nextBound = numeric_limits<int>::max();
        vector<SearchFrame> stack;
        vector<Cube::Move> path;
        stack.push_back({start, 0, -1, false, 0, {}});

        while (!stack.empty()) {
            SearchFrame& frame = stack.back();

            if (frame.cube.isSolved()) {
                result.moves = path;
                result.solutionDepth = static_cast<uint8_t>(path.size());
                return result;
            }

            if (!frame.expanded) {
                frame.expanded = true;
                frame.children.clear();
                ++result.expandedNodes;

                if (frame.depth < bound) {
                    for (int rawMove = 0; rawMove < 18; ++rawMove) {
                        const auto move = static_cast<Cube::Move>(rawMove);
                        if (shouldPrune(move, frame.previousFace)) continue;

                        Cube child = frame.cube;
                        child.apply(move);
                        const uint8_t childHeuristic = heuristic.estimate(child);
                        const int score = frame.depth + 1 + childHeuristic;

                        if (score <= bound) {
                            frame.children.push_back({child, move, childHeuristic});
                        } else {
                            nextBound = min(nextBound, score);
                        }
                    }

                    // All children have the same g cost, so ordering by h is
                    // equivalent to ordering by f. The explicit stack then
                    // visits the most promising state first.
                    stable_sort(
                        frame.children.begin(), frame.children.end(),
                        [](const ChildNode& left, const ChildNode& right) {
                            if (left.heuristic != right.heuristic) {
                                return left.heuristic < right.heuristic;
                            }
                            return static_cast<int>(left.move) <
                                   static_cast<int>(right.move);
                        });
                }
            }

            if (frame.nextChild < frame.children.size()) {
                ChildNode child = std::move(frame.children[frame.nextChild++]);
                path.resize(frame.depth);
                path.push_back(child.move);
                stack.push_back({
                    std::move(child.cube),
                    static_cast<uint8_t>(frame.depth + 1),
                    Cube::face(child.move),
                    false,
                    0,
                    {},
                });
                continue;
            }

            stack.pop_back();
        }

        if (nextBound == numeric_limits<int>::max()) {
            throw runtime_error("IDA* exhausted the reachable search space");
        }
        bound = nextBound;
    }

    throw runtime_error("No solution was found within God's number (20 HTM moves)");
}
