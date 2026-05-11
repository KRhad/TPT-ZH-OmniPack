#pragma once
#include <array>

//Directions in counter-clockwise order starting from the top:
// 107
// 2.6
// 345
constexpr std::array<std::array<int, 2>, 8> dir3x3 = {{ {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1} }};

// Positions of info in the ctype field
constexpr int PLNT_PHASE = 1;
constexpr int PLNT_DIR = 3;
constexpr int PLNT_COLOUR = 6;
constexpr int PLNT_LIFE = 12;

// Total number of bits used
constexpr int PLNT_TOTAL_CTP = 20;
constexpr int PLNT_TOTAL_TMP = 15;

// Detect down direction using gravity. Returns 8 when there's no direction.
class Simulation;
int detectDown(Simulation *sim, int x, int y);
