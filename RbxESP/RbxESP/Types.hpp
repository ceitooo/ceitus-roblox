#pragma once
#include <cstdint>

struct Vector2 { float x, y; };
struct Vector3 { float x, y, z; };
struct Color3 { float r, g, b; };

struct Matrix3x3 {
    float m[3][3];
};

struct ViewMatrix_t {
    float m[4][4];
};

struct UDim2 { float x, y, ox, oy; };