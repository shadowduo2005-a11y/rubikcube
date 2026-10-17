#ifndef CUBELAB_THISTLETHWAITE_PHASE_COORDINATES_H
#define CUBELAB_THISTLETHWAITE_PHASE_COORDINATES_H

#include "../Model/CubieCube.h"

#include <cstddef>
#include <cstdint>

using namespace std;

class PhaseCoordinates {
public:
    static constexpr size_t GROUP1_STATES = 2048;
    static constexpr size_t GROUP2_STATES = 1082565;
    static constexpr size_t GROUP3_STATES = 352800;
    static constexpr size_t GROUP4_STATES = 663552;

    static uint32_t group1(const Cube& cube);
    static uint32_t group2(const Cube& cube);
    static uint32_t group3(const Cube& cube);
    static uint32_t group4(const Cube& cube);

    static bool inGroup1(const Cube& cube);
    static bool inGroup2(const Cube& cube);
    static uint8_t cornerParity(const Cube& cube);
};

#endif
