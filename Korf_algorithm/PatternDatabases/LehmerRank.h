#ifndef CUBELAB_KORF_LEHMER_RANK_H
#define CUBELAB_KORF_LEHMER_RANK_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

using namespace std;

constexpr uint32_t permutations(uint32_t n, uint32_t k) {
    uint32_t result = 1;
    for (uint32_t value = n - k + 1; value <= n; ++value) result *= value;
    return result;
}

// Perfect lexicographic rank for K values selected without replacement from
// [0, N). The Lehmer digits are evaluated in O(K) with a small popcount table.
template <size_t N, size_t K = N>
class LehmerRank {
public:
    LehmerRank() {
        onesCount[0] = 0;
        for (size_t mask = 1; mask < onesCount.size(); ++mask) {
            onesCount[mask] = static_cast<uint8_t>(
                onesCount[mask >> 1] + (mask & 1U));
        }
        for (size_t position = 0; position < K; ++position) {
            weights[position] = permutations(
                static_cast<uint32_t>(N - 1 - position),
                static_cast<uint32_t>(K - 1 - position));
        }
    }

    uint32_t rankWithoutReplacement(const array<uint8_t, K>& values) const {
        uint32_t result = 0;
        size_t seen = 0;
        for (size_t position = 0; position < K; ++position) {
            const uint8_t value = values[position];
            if (value >= N || (seen & (size_t{1} << value)) != 0) {
                throw invalid_argument("Lehmer rank requires distinct in-range values");
            }
            const size_t lowerMask = value == 0 ? 0 : (size_t{1} << value) - 1;
            const uint32_t smallerAlreadySeen = onesCount[seen & lowerMask];
            const uint32_t digit = value - smallerAlreadySeen;
            result += digit * weights[position];
            seen |= size_t{1} << value;
        }
        return result;
    }

private:
    array<uint8_t, size_t{1} << N> onesCount{};
    array<uint32_t, K> weights{};
};

template <uint32_t Base, size_t K>
uint32_t rankWithReplacement(const array<uint8_t, K>& values) {
    uint32_t rank = 0;
    for (uint8_t value : values) {
        if (value >= Base) throw invalid_argument("Orientation digit is out of range");
        rank = rank * Base + value;
    }
    return rank;
}

#endif
