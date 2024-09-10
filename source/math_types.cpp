#include "math_types.h"
#include "common.h"

#include <math.h>

/*
    --- Utils ---
*/

float clamp(float value, float min, float max) {
    ASSERT(min <= max);

    if(value <= min) {
        return min;
    } else if(value >= max) {
        return max;
    } else {
        return value;
    }
}

// NOTE: Value must be between <left-range, max+range>, -range = right-left, to work fine
// TODO: Make better version of this
float wrap(float value, float left, float right) {
    ASSERT(left < right);
    
    if(value < left) {
        return right - (left - value);
    } else if(value > right) {
        return left + (value - right);
    } else {
        return value;
    }
}

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
    --- vec3 ---
*/

vec3 vec3::make(float x, float y, float z) {
    return vec3{ x, y, z };
}

vec3 vec3::make(float xyz) {
    return vec3{ xyz, xyz, xyz };
}

vec3 vec3::make(const vec2 &v, float z) {
    return vec3{ v.x, v.y, z };
}

vec3 vec3::zero(void) {
    return vec3{ 0.0f, 0.0f, 0.0f, };
}

float vec3::length_sq(const vec3 &vec) {
    return vec3::dot(vec, vec);
}

float vec3::length(const vec3 &vec) {
    return sqrtf(vec3::length_sq(vec));
}

vec3 vec3::normalize(const vec3 &vec) {
    const float length = vec3::length(vec);
    return vec / length;
}

/* TODO Copied, learn */
vec3 vec3::cross(const vec3 &a, const vec3 &b) {
    return vec3{
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

float vec3::dot(const vec3 &a, const vec3 &b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}


vec3 operator + (const vec3 &l, const vec3 r) {
    return vec3{ l.x + r.x, l.y + r.y, l.z + r.z };
}

vec3 operator - (const vec3 &l, const vec3 r) {
    return vec3{ l.x - r.x, l.y - r.y, l.z - r.z };
}

vec3 operator / (const vec3 &l, const float r) {
    return vec3{ l.x / r, l.y / r, l.z / r };
}

/*
    --- mat4 ---
*/

static inline mat4 mat4_from_linmath(float values[4][4]) {
    return mat4{
        values[0][0], values[0][1], values[0][2], values[0][3],
        values[1][0], values[1][1], values[1][2], values[1][3],
        values[2][0], values[2][1], values[2][2], values[2][3],
        values[3][0], values[3][1], values[3][2], values[3][3],
    };
}

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

mat4 mat4::perspective(float fov, float aspect, float near, float far) {
    /* NOTE: Degrees are an unhandy unit to work with.
	 * linmath.h uses radians for everything! */
	float const a = 1.f / tanf(fov / 2.f);

    mat4 m = mat4::identity();

	m.e[0][0] = a / aspect;
	m.e[0][1] = 0.f;
	m.e[0][2] = 0.f;
	m.e[0][3] = 0.f;

	m.e[1][0] = 0.f;
	m.e[1][1] = a;
	m.e[1][2] = 0.f;
	m.e[1][3] = 0.f;

	m.e[2][0] = 0.f;
	m.e[2][1] = 0.f;
	m.e[2][2] = -((far + near) / (far - near));
	m.e[2][3] = -1.f;

	m.e[3][0] = 0.f;
	m.e[3][1] = 0.f;
	m.e[3][2] = -((2.f * far * near) / (far - near));
	m.e[3][3] = 0.f;

    return mat4_from_linmath(m.e);
}

mat4 mat4::look_at(vec3 eye, vec3 focus, vec3 up_vec) {
    vec3 z_axis = vec3::normalize(eye - focus);
    vec3 x_axis = vec3::normalize(vec3::cross(up_vec, z_axis));
    vec3 y_axis = vec3::cross(z_axis, x_axis);

    mat4 view = {
        x_axis.x, y_axis.x, z_axis.x, 0.0f,
        x_axis.y, y_axis.y, z_axis.y, 0.0f,
        x_axis.z, y_axis.z, z_axis.z, 0.0f,
        0.0f - vec3::dot(x_axis, eye), 0.0f - vec3::dot(y_axis, eye), 0.0f - vec3::dot(z_axis, eye), 1.0f,
    };
    //return f;
    return mat4_from_linmath(view.e);
}

mat4 mat4::translate(const vec3 &vec) {
    return mat4{
        1.0f, 0.0f, 0.0f, vec.x,
        0.0f, 1.0f, 0.0f, vec.y,
        0.0f, 0.0f, 1.0f, vec.z,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
}
