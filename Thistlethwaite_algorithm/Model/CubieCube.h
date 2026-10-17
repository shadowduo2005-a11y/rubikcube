#ifndef CUBELAB_THISTLETHWAITE_CUBIE_CUBE_H
#define CUBELAB_THISTLETHWAITE_CUBIE_CUBE_H

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

// Standard cubie order shared conceptually with the Korf engine and the
// independent JavaScript sticker model.
//
// Edges:  UB UR UF UL FR FL BL BR DF DL DB DR
// Corners: ULB URB URF ULF DLF DLB DRB DRF
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
