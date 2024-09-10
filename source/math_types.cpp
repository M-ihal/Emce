#include "math_types.h"

/*
    --- vec2 ---
*/

vec2 vec2::make(float x, float y) {
    return vec2{ x, y };
}

vec2 vec2::make(float xy) {
    return vec2{ xy, xy };
}

vec2 vec2::zero(void) {
    return vec2{ 0.0f, 0.0f };
}

/*
    --- mat4 ---
*/

mat4 mat4::identity(void) {
    return mat4{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
}

/* TODO Copied from other project - Re-check this */
mat4 mat4::orthographic(float left, float bottom, float right, float top, float near, float far) {
    mat4 proj = mat4::identity();
    proj.e00 = 2.0f / (right - left);
    proj.e11 = 2.0f / (top - bottom);
    proj.e22 = 2.0f / (far - near);
    proj.e03 = -((right + left) / (right - left));
    proj.e13 = -((top + bottom) / (top - bottom));
    proj.e23 = -((far + near) / (far - near));
    return proj;
}
