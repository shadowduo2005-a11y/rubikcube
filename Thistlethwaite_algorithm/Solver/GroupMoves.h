#ifndef CUBELAB_THISTLETHWAITE_GROUP_MOVES_H
#define CUBELAB_THISTLETHWAITE_GROUP_MOVES_H

#include "../Model/CubieCube.h"
#include <vector>

class GroupMoves {
public:
    static const std::vector<Cube::Move>& group0() {
        static const std::vector<Cube::Move> moves = {
            Cube::Move::U, Cube::Move::U_PRIME, Cube::Move::U2,
            Cube::Move::D, Cube::Move::D_PRIME, Cube::Move::D2,
            Cube::Move::L, Cube::Move::L_PRIME, Cube::Move::L2,
            Cube::Move::R, Cube::Move::R_PRIME, Cube::Move::R2,
            Cube::Move::F, Cube::Move::F_PRIME, Cube::Move::F2,
            Cube::Move::B, Cube::Move::B_PRIME, Cube::Move::B2
        };
        return moves;
    }

    static const std::vector<Cube::Move>& group1() {
        static const std::vector<Cube::Move> moves = {
            Cube::Move::U, Cube::Move::U_PRIME, Cube::Move::U2,
            Cube::Move::D, Cube::Move::D_PRIME, Cube::Move::D2,
            Cube::Move::L, Cube::Move::L_PRIME, Cube::Move::L2,
            Cube::Move::R, Cube::Move::R_PRIME, Cube::Move::R2,
            Cube::Move::F2, Cube::Move::B2
        };
        return moves;
    }

    static const std::vector<Cube::Move>& group2() {
        static const std::vector<Cube::Move> moves = {
            Cube::Move::U, Cube::Move::U_PRIME, Cube::Move::U2,
            Cube::Move::D, Cube::Move::D_PRIME, Cube::Move::D2,
            Cube::Move::L2, Cube::Move::R2,
            Cube::Move::F2, Cube::Move::B2
        };
        return moves;
    }

    static const std::vector<Cube::Move>& group3() {
        static const std::vector<Cube::Move> moves = {
            Cube::Move::U2, Cube::Move::D2,
            Cube::Move::L2, Cube::Move::R2,
            Cube::Move::F2, Cube::Move::B2
        };
        return moves;
    }
};

#endif
