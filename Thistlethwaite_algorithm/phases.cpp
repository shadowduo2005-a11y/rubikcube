#include "phases.h"

bool phase1(const Cube& c) {
    for (int i = 0; i < 12; i++)
        if (c.eo[i]) return false;
    return true;
}

bool phase2(const Cube& c) {
    for (int i = 0; i < 8; i++)
        if (c.co[i]) return false;
    for (int i = 8; i < 12; i++)
        if (c.ep[i] < 8) return false;
    return true;
}

bool phase3(const Cube& c) {
    for (int i = 0; i < 8; i++)
        if (c.cp[i] != i) return false;
    return true;
}

bool phase4(const Cube& c) {
    return c == solvedCube();
}
