#include "PhaseCoordinates.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <vector>

using namespace std;

namespace {

constexpr uint32_t choose(uint32_t n, uint32_t k) {
    if (k > n) return 0;
    if (k > n - k) k = n - k;
    uint32_t result = 1;
    for (uint32_t divisor = 1; divisor <= k; ++divisor) {
        result = result * (n - k + divisor) / divisor;
    }
    return result;
}

template <size_t K>
uint32_t combinationRank(const array<uint8_t, K>& values, uint8_t n) {
    uint32_t rank = 0;
    int previous = -1;
    for (size_t position = 0; position < K; ++position) {
        const int value = values[position];
        if (value <= previous || value >= n) {
            throw logic_error("Combination coordinate is not sorted and distinct");
        }
        for (int skipped = previous + 1; skipped < value; ++skipped) {
            rank += choose(
                static_cast<uint32_t>(n - 1 - skipped),
                static_cast<uint32_t>(K - 1 - position));
        }
        previous = value;
    }
    return rank;
}

template <size_t K>
uint32_t permutationRank(const array<uint8_t, K>& values, uint8_t alphabetSize) {
    uint32_t rank = 0;
    uint16_t seen = 0;
    for (size_t position = 0; position < K; ++position) {
        const uint8_t value = values[position];
        if (value >= alphabetSize || (seen & (uint16_t{1} << value)) != 0) {
            throw logic_error("Permutation coordinate contains an invalid value");
        }
        uint32_t smallerSeen = 0;
        for (uint8_t candidate = 0; candidate < value; ++candidate) {
            smallerSeen += (seen & (uint16_t{1} << candidate)) != 0;
        }
        const uint32_t digit = value - smallerSeen;
        uint32_t weight = 1;
        const uint32_t remainingAlphabet = alphabetSize - 1 - position;
        const uint32_t remainingDigits = K - 1 - position;
        for (uint32_t factor = 0; factor < remainingDigits; ++factor) {
            weight *= remainingAlphabet - factor;
        }
        rank += digit * weight;
        seen |= uint16_t{1} << value;
    }
    return rank;
}

array<uint8_t, 8> cornerPositions(const Cube& cube) {
    array<uint8_t, 8> positions{};
    for (uint8_t position = 0; position < cube.corners().size(); ++position) {
        positions[cube.corners()[position].index] = position;
    }
    return positions;
}

uint32_t orderedCornerPairRank(const Cube& cube) {
    // These four fixed pairs are preserved as pairs by the half-turn group.
    static constexpr array<array<uint8_t, 2>, 4> PIECE_PAIRS = {{
        {{0, 2}},  // ULB, URF
        {{4, 6}},  // DLF, DRB
        {{1, 3}},  // URB, ULF
        {{5, 7}},  // DLB, DRF
    }};

    const auto positions = cornerPositions(cube);
    vector<uint8_t> available = {0, 1, 2, 3, 4, 5, 6, 7};
    uint32_t rank = 0;

    for (size_t pairIndex = 0; pairIndex < 3; ++pairIndex) {
        array<uint8_t, 2> mapped{};
        for (size_t member = 0; member < 2; ++member) {
            const uint8_t absolutePosition = positions[PIECE_PAIRS[pairIndex][member]];
            const auto found = find(available.begin(), available.end(), absolutePosition);
            if (found == available.end()) {
                throw logic_error("Corner-pair coordinate is inconsistent");
            }
            mapped[member] = static_cast<uint8_t>(distance(available.begin(), found));
        }
        sort(mapped.begin(), mapped.end());
        const uint32_t digit = combinationRank(mapped, static_cast<uint8_t>(available.size()));
        rank = rank * choose(static_cast<uint32_t>(available.size()), 2) + digit;

        const uint8_t firstAbsolute = positions[PIECE_PAIRS[pairIndex][0]];
        const uint8_t secondAbsolute = positions[PIECE_PAIRS[pairIndex][1]];
        available.erase(
            remove_if(
                available.begin(), available.end(),
                [firstAbsolute, secondAbsolute](uint8_t value) {
                    return value == firstAbsolute || value == secondAbsolute;
                }),
            available.end());
    }
    return rank;
}

}  // namespace

uint32_t PhaseCoordinates::group1(const Cube& cube) {
    uint32_t rank = 0;
    for (size_t position = 0; position < 11; ++position) {
        rank = rank * 2 + cube.edges()[position].orientation;
    }
    return rank;
}

uint32_t PhaseCoordinates::group2(const Cube& cube) {
    static constexpr array<bool, 12> IS_E_SLICE = {
        false, false, false, false, true, true,
        true, true, false, false, false, false,
    };

    array<uint8_t, 4> eSlicePositions{};
    size_t found = 0;
    for (uint8_t position = 0; position < cube.edges().size(); ++position) {
        if (IS_E_SLICE[cube.edges()[position].index]) {
            if (found >= eSlicePositions.size()) {
                throw logic_error("Too many E-slice edges in cube coordinate");
            }
            eSlicePositions[found++] = position;
        }
    }
    if (found != eSlicePositions.size()) {
        throw logic_error("Cube is missing an E-slice edge");
    }

    uint32_t orientationRank = 0;
    for (size_t position = 0; position < 7; ++position) {
        orientationRank = orientationRank * 3 + cube.corners()[position].orientation;
    }
    return combinationRank(eSlicePositions, 12) * 2187U + orientationRank;
}

uint8_t PhaseCoordinates::cornerParity(const Cube& cube) {
    uint8_t parity = 0;
    for (size_t left = 0; left < cube.corners().size(); ++left) {
        for (size_t right = left + 1; right < cube.corners().size(); ++right) {
            parity ^= cube.corners()[left].index > cube.corners()[right].index;
        }
    }
    return parity;
}

uint32_t PhaseCoordinates::group3(const Cube& cube) {
    // Once G2 is reached, positions 4..7 contain the E-slice pieces. Map the
    // remaining eight positions to [0,7] and rank the four M-slice pieces.
    static constexpr array<int8_t, 12> NON_E_POSITION_MAP = {
        0, 1, 2, 3, -1, -1, -1, -1, 4, 5, 6, 7,
    };
    static constexpr array<bool, 12> IS_M_SLICE = {
        true, false, true, false, false, false,
        false, false, true, false, true, false,
    };

    array<uint8_t, 4> mSlicePositions{};
    size_t found = 0;
    for (uint8_t position = 0; position < cube.edges().size(); ++position) {
        if (!IS_M_SLICE[cube.edges()[position].index]) continue;
        const int8_t mapped = NON_E_POSITION_MAP[position];
        if (mapped < 0 || found >= mSlicePositions.size()) {
            throw logic_error("G3 coordinate requires a cube already in G2");
        }
        mSlicePositions[found++] = static_cast<uint8_t>(mapped);
    }
    if (found != mSlicePositions.size()) {
        throw logic_error("Cube is missing an M-slice edge");
    }
    sort(mSlicePositions.begin(), mSlicePositions.end());

    const uint32_t edgeRank = combinationRank(mSlicePositions, 8);
    const uint32_t pairRank = orderedCornerPairRank(cube);
    return (edgeRank * 2520U + pairRank) * 2U + cornerParity(cube);
}

uint32_t PhaseCoordinates::group4(const Cube& cube) {
    static constexpr array<uint8_t, 4> M_POSITIONS = {0, 2, 8, 10};
    static constexpr array<uint8_t, 4> S_POSITIONS = {1, 3, 9, 11};
    static constexpr array<uint8_t, 2> E_POSITIONS = {4, 5};
    static constexpr array<int8_t, 12> EDGE_MAP = {
        0, 0, 1, 1, 0, 1, 2, 3, 2, 2, 3, 3,
    };

    array<uint8_t, 4> mPermutation{};
    array<uint8_t, 4> sPermutation{};
    array<uint8_t, 2> ePartialPermutation{};
    for (size_t index = 0; index < M_POSITIONS.size(); ++index) {
        mPermutation[index] = static_cast<uint8_t>(
            EDGE_MAP[cube.edges()[M_POSITIONS[index]].index]);
        sPermutation[index] = static_cast<uint8_t>(
            EDGE_MAP[cube.edges()[S_POSITIONS[index]].index]);
    }
    for (size_t index = 0; index < E_POSITIONS.size(); ++index) {
        ePartialPermutation[index] = static_cast<uint8_t>(
            EDGE_MAP[cube.edges()[E_POSITIONS[index]].index]);
    }

    const uint32_t edgeRank =
        permutationRank(mPermutation, 4) * 288U +
        permutationRank(sPermutation, 4) * 12U +
        permutationRank(ePartialPermutation, 4);

    static constexpr array<uint8_t, 4> FIRST_TETRAD_POSITIONS = {0, 2, 4, 6};
    static constexpr array<int8_t, 8> CORNER_MAP = {0, 0, 1, 1, 2, 2, 3, 3};
    array<uint8_t, 4> firstTetradPermutation{};
    for (size_t index = 0; index < FIRST_TETRAD_POSITIONS.size(); ++index) {
        firstTetradPermutation[index] = static_cast<uint8_t>(
            CORNER_MAP[cube.corners()[FIRST_TETRAD_POSITIONS[index]].index]);
    }
    const uint8_t secondTetradPiece =
        static_cast<uint8_t>(CORNER_MAP[cube.corners()[1].index]);
    const uint32_t cornerRank =
        permutationRank(firstTetradPermutation, 4) * 4U + secondTetradPiece;

    return edgeRank * 96U + cornerRank;
}

bool PhaseCoordinates::inGroup1(const Cube& cube) {
    for (const Cubie edge : cube.edges()) {
        if (edge.orientation != 0) return false;
    }
    return true;
}

bool PhaseCoordinates::inGroup2(const Cube& cube) {
    if (!inGroup1(cube)) return false;
    for (const Cubie corner : cube.corners()) {
        if (corner.orientation != 0) return false;
    }
    for (size_t position = 4; position <= 7; ++position) {
        const uint8_t piece = cube.edges()[position].index;
        if (piece < 4 || piece > 7) return false;
    }
    return true;
}
