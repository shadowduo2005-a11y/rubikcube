#ifndef CUBELAB_THISTLETHWAITE_GROUP_MOVES_H
#define CUBELAB_THISTLETHWAITE_GROUP_MOVES_H

#include "../Model/CubieCube.h"

#include <vector>

using namespace std;

class GroupMoves {
public:
    // G0 -> G1: all 18 twists.
    static const vector<Cube::Move>& group0() {
        static const vector<Cube::Move> moves = {
            Cube::Move::U, Cube::Move::U_PRIME, Cube::Move::U2,
            Cube::Move::D, Cube::Move::D_PRIME, Cube::Move::D2,
            Cube::Move::L, Cube::Move::L_PRIME, Cube::Move::L2,
            Cube::Move::R, Cube::Move::R_PRIME, Cube::Move::R2,
            Cube::Move::F, Cube::Move::F_PRIME, Cube::Move::F2,
            Cube::Move::B, Cube::Move::B_PRIME, Cube::Move::B2,
        };
        return moves;
    }

    // G1 -> G2: F/B quarter turns are forbidden.
    static const vector<Cube::Move>& group1() {
        static const vector<Cube::Move> moves = {
            Cube::Move::U, Cube::Move::U_PRIME, Cube::Move::U2,
            Cube::Move::D, Cube::Move::D_PRIME, Cube::Move::D2,
            Cube::Move::L, Cube::Move::L_PRIME, Cube::Move::L2,
            Cube::Move::R, Cube::Move::R_PRIME, Cube::Move::R2,
            Cube::Move::F2, Cube::Move::B2,
        };
        return moves;
    }

    // G2 -> G3: only U/D quarter turns remain; all side turns are halves.
    static const vector<Cube::Move>& group2() {
        static const vector<Cube::Move> moves = {
            Cube::Move::U, Cube::Move::U_PRIME, Cube::Move::U2,
            Cube::Move::D, Cube::Move::D_PRIME, Cube::Move::D2,
            Cube::Move::L2, Cube::Move::R2, Cube::Move::F2, Cube::Move::B2,
        };
        return moves;
    }

    // G3 -> solved: half turns only.
    static const vector<Cube::Move>& group3() {
        static const vector<Cube::Move> moves = {
            Cube::Move::U2, Cube::Move::D2, Cube::Move::L2,
            Cube::Move::R2, Cube::Move::F2, Cube::Move::B2,
        };
        return moves;
    }
};

#endif
