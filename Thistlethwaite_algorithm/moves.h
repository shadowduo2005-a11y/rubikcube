#ifndef MOVES_H
#define MOVES_H

#include "cube.h"
#include <vector>
#include <string>

struct Move {
    int ep[12], eo[12];
    int cp[8], co[8];
};

extern std::vector<Move> moves;
extern std::vector<std::string> moveNames;

Cube applyMove(const Cube& c, const Move& m);
void initMoves();

#endif
