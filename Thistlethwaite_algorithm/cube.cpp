#include "cube.h"

bool Cube::operator==(const Cube& o) const {
    for (int i = 0; i < 12; i++)
        if (ep[i] != o.ep[i] || eo[i] != o.eo[i]) return false;
    for (int i = 0; i < 8; i++)
        if (cp[i] != o.cp[i] || co[i] != o.co[i]) return false;
    return true;
}

Cube solvedCube() {
    Cube c;
    for (int i = 0; i < 12; i++) c.ep[i] = i, c.eo[i] = 0;
    for (int i = 0; i < 8; i++) c.cp[i] = i, c.co[i] = 0;
    return c;
}

size_t hashCube(const Cube& c) {
    size_t h = 0;
    for (int i = 0; i < 12; i++)
        h = h * 31 + c.ep[i] * 2 + c.eo[i];
    for (int i = 0; i < 8; i++)
        h = h * 31 + c.cp[i] * 3 + c.co[i];
    return h;
}
