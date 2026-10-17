#include "moves.h"

std::vector<Move> moves;
std::vector<std::string> moveNames;

static Move U = {{
    3,0,1,2, 4,5,6,7, 8,9,10,11
},{0,0,0,0, 0,0,0,0, 0,0,0,0},{
    3,0,1,2, 4,5,6,7
},{0,0,0,0, 0,0,0,0}};

static Move D = {{
    0,1,2,3, 5,6,7,4, 8,9,10,11
},{0,0,0,0, 0,0,0,0, 0,0,0,0},{
    0,1,2,3, 5,6,7,4
},{0,0,0,0, 0,0,0,0}};

static Move L = {{
    0,1,10,3, 4,5,9,7, 8,2,6,11
},{0,0,0,0, 0,0,0,0, 0,0,0,0},{
    0,2,6,3, 4,1,5,7
},{0,1,2,0, 0,2,1,0}};

static Move R = {{
    8,1,2,3, 11,5,6,7, 4,9,10,0
},{0,0,0,0, 0,0,0,0, 0,0,0,0},{
    4,1,2,0, 7,5,6,3
},{2,0,0,1, 1,0,0,2}};

static Move F = {{
    0,9,2,3, 4,8,6,7, 1,5,10,11
},{0,1,0,0, 0,1,0,0, 1,1,0,0},{
    4,0,2,3, 5,1,6,7
},{1,2,0,0, 2,1,0,0}};

static Move B = {{
    0,1,2,11, 4,5,6,10, 8,9,3,7
},{0,0,0,1, 0,0,0,1, 0,0,1,1},{
    0,1,6,2, 4,5,7,3
},{0,0,1,2, 0,0,2,1}};

Cube applyMove(const Cube& c, const Move& m) {
    Cube r;
    for (int i = 0; i < 12; i++) {
        r.ep[i] = c.ep[m.ep[i]];
        r.eo[i] = (c.eo[m.ep[i]] + m.eo[i]) % 2;
    }
    for (int i = 0; i < 8; i++) {
        r.cp[i] = c.cp[m.cp[i]];
        r.co[i] = (c.co[m.cp[i]] + m.co[i]) % 3;
    }
    return r;
}

static Move powMove(Move m, int p) {
    Cube c = solvedCube();
    for (int i = 0; i < p; i++) c = applyMove(c, m);
    Move r{};
    for (int i = 0; i < 12; i++) r.ep[i] = c.ep[i], r.eo[i] = c.eo[i];
    for (int i = 0; i < 8; i++) r.cp[i] = c.cp[i], r.co[i] = c.co[i];
    return r;
}

void initMoves() {
    Move base[] = {U,D,L,R,F,B};
    std::string names[] = {"U","D","L","R","F","B"};

    for (int i = 0; i < 6; i++) {
        moves.push_back(base[i]);
        moveNames.push_back(names[i]);

        moves.push_back(powMove(base[i],2));
        moveNames.push_back(names[i]+"2");

        moves.push_back(powMove(base[i],3));
        moveNames.push_back(names[i]+"'");
    }
}
