#ifndef CUBE_H
#define CUBE_H

#include <cstddef>

struct Cube {
    int ep[12], eo[12];
    int cp[8], co[8];

    bool operator==(const Cube& o) const;
};

Cube solvedCube();
size_t hashCube(const Cube& c);

#endif
