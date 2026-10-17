#ifndef CUBELAB_KORF_CUBIE_CUBE_H
#define CUBELAB_KORF_CUBIE_CUBE_H

#include <array>
#include <cstdint>
#include <string>

using namespace std;

struct Cubie {
    uint8_t index;
    uint8_t orientation;

    bool operator==(const Cubie& other) const {
        return index == other.index && orientation == other.orientation;
    }
};

// Index-orientation representation used by the optimal solver.
//
// Edge positions/cubies:
//   0 UB, 1 UR, 2 UF, 3 UL, 4 FR, 5 FL,
//   6 BL, 7 BR, 8 DF, 9 DL, 10 DB, 11 DR
// Corner positions/cubies:
//   0 ULB, 1 URB, 2 URF, 3 ULF,
//   4 DLF, 5 DLB, 6 DRB, 7 DRF
class Cube {
public:
    enum class Move : uint8_t {
        U, U_PRIME, U2,
        D, D_PRIME, D2,
        L, L_PRIME, L2,
        R, R_PRIME, R2,
        F, F_PRIME, F2,
        B, B_PRIME, B2,
    };

    Cube();

    const array<Cubie, 8>& corners() const { return cornerCubies; }
    const array<Cubie, 12>& edges() const { return edgeCubies; }

    Cube& apply(Move move);
    bool isSolved() const;
    bool isValid() const;

    bool operator==(const Cube& other) const {
        return cornerCubies == other.cornerCubies && edgeCubies == other.edgeCubies;
    }

    static int face(Move move) { return static_cast<int>(move) / 3; }
    static Move inverse(Move move);
    static string moveName(Move move);
    static bool parseMove(const string& token, Move& move);

private:
    array<Cubie, 8> cornerCubies;
    array<Cubie, 12> edgeCubies;

    void applyQuarterTurn(int faceIndex);
};

#endif
