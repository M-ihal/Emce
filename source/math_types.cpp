#include "math_types.h"
#include "common.h"

#include <math.h>

/*
    --- Utils ---
*/

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

double wrap(double value, double left, double right) {
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

vec2 vec2::make(const vec2i &v) {
    return vec2{ (float)v.x, (float)v.y };
}

vec2 vec2::make_xz(const vec3 &v) {
    return vec2{ v.x, v.z };
}

vec2 vec2::zero(void) {
    return vec2{ 0.0f, 0.0f };
}

vec2 vec2::absolute(const vec2 &v) {
    return vec2{ ABS(v.x), ABS(v.y) };
}

float vec2::length_sq(const vec2 &v) {
    return vec2::dot(v, v);
}

float vec2::length(const vec2 &v) {
    return sqrtf(vec2::length_sq(v));
}

float vec2::dot(const vec2 &a, const vec2 &b) {
    return a.x * b.x + a.y * b.y;
}

vec2 vec2::normalize(const vec2 &v) {
    const float length = vec2::length(v);
    return vec2{
        .x = v.x / length,
        .y = v.y / length
    };
}

vec2 operator + (const vec2 &l, const vec2 &r) {
    return vec2{ l.x + r.x, l.y + r.y };
}

vec2 operator - (const vec2 &l, const vec2 &r) {
    return vec2{ l.x - r.x, l.y - r.y };
}

vec2 operator * (const vec2 &l, float r) {
    return vec2{ l.x * r, l.y * r };
}

vec2 &operator *= (vec2 &l, float r) {
    l = l * r;
    return l;
}

/*
    --- vec2d ---
*/

vec2d vec2d::make(double x, double y) {
    return vec2d{ x, y };
}

vec2d vec2d::make(double xy) {
    return vec2d{ xy, xy };
}

vec2d vec2d::make(const vec2i &v) {
    return vec2d{ (double)v.x, (double)v.y };
}

vec2d vec2d::make_xz(const vec3d &v) {
    return vec2d{ v.x, v.z };
}

vec2d vec2d::zero(void) {
    return vec2d{ 0.0, 0.0 };
}

vec2d vec2d::absolute(const vec2d &v) {
    return vec2d{ ABS(v.x), ABS(v.y) };
}

double vec2d::length_sq(const vec2d &v) {
    return vec2d::dot(v, v);
}

double vec2d::length(const vec2d &v) {
    return sqrtf(vec2d::length_sq(v));
}

double vec2d::dot(const vec2d &a, const vec2d &b) {
    return a.x * b.x + a.y * b.y;
}

vec2d vec2d::normalize(const vec2d &v) {
    const double length = vec2d::length(v);
    return vec2d{
        .x = v.x / length,
        .y = v.y / length
    };
}

vec2d operator + (const vec2d &l, const vec2d &r) {
    return vec2d{ l.x + r.x, l.y + r.y };
}

vec2d operator - (const vec2d &l, const vec2d &r) {
    return vec2d{ l.x - r.x, l.y - r.y };
}

vec2d operator * (const vec2d &l, const vec2d &r) {
    return vec2d{ l.x * r.x, l.y * r.y };
}

vec2d operator * (const vec2d &l, double r) {
    return vec2d{ l.x * r, l.y * r };
}

vec2d &operator *= (vec2d &l, double r) {
    l = l * r;
    return l;
}

/*
   --- vec2i ---
   */

vec2i vec2i::make_xz(const struct vec3i &v) {
    return vec2i{ v.x, v.z };
}

vec2i vec2i::zero(void) {
    return vec2i{ 0, 0 };
}

vec2i vec2i::absolute(const vec2i &v) {
    return vec2i{ ABS(v.x), ABS(v.y) };
}

vec2i operator + (const vec2i &l, const vec2i &r) {
    return vec2i{ l.x + r.x, l.y + r.y };
}

vec2i operator - (const vec2i &l, const vec2i &r) {
    return vec2i{ l.x - r.x, l.y - r.y };

}

vec2i operator / (const vec2i &l, int32_t r) {
    return vec2i{ l.x / r, l.y / r };
}

bool operator == (const vec2i &l, const vec2i &r) {
    return l.x == r.x && l.y == r.y;
}

bool operator != (const vec2i &l, const vec2i &r) {
    return !(l == r);
}

/*
    --- vec3 ---
*/

vec3 vec3::zero(void) {
    return vec3{ 0.0f, 0.0f, 0.0f, };
}

vec2 vec3::get_xz(void) const {
    return vec2{ x, z };
}

vec3 vec3::make(float x, float y, float z) {
    return vec3{ x, y, z };
}

vec3 vec3::make(float xyz) {
    return vec3{ xyz, xyz, xyz };
}

vec3 vec3::make(const vec2 &v, float z) {
    return vec3{ v.x, v.y, z };
}

vec3 vec3::make(const vec3i &v) {
    return vec3{ (float)v.x, (float)v.y, (float)v.z };
}

vec3 vec3::make(const struct vec3d &v) {
    return vec3{ (float)v.x, (float)v.y, (float)v.z };
}

vec3 vec3::absolute(const vec3 &vec) {
    return vec3{ ABS(vec.x), ABS(vec.y), ABS(vec.z) };
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

vec3 operator * (const vec3 &l, const vec3 r) {
    return vec3{ l.x * r.x, l.y * r.y, l.z * r.z };
}

vec3 operator * (const vec3 &l, const float r) {
    return vec3{ l.x * r, l.y * r, l.z * r };
}

vec3 operator * (const float l, const vec3 &r) {
    return r * l;
}

vec3 operator / (const vec3 &l, const vec3 r) {
    return vec3{ l.x / r.x, l.y / r.y, l.z / r.z };
}

vec3 operator / (const vec3 &l, const float r) {
    return vec3{ l.x / r, l.y / r, l.z / r };
}

vec3 &operator += (vec3 &l, const vec3 &r) {
    l = l + r;
    return l;
}

vec3 &operator -= (vec3 &l, const vec3 &r) {
    l = l - r;
    return l;
}

vec3 &operator *= (vec3 &l, const vec3 &r) {
    l = l * r;
    return l;
}

vec3 &operator *= (vec3 &l, const float &r) {
    l = l * r;
    return l;
}

/*
    --- vec3d ---
*/

vec3d vec3d::zero(void) {
    return vec3d{ 0.0, 0.0, 0.0, };
}

vec3d vec3d::make(double x, double y, double z) {
    return vec3d{ x, y, z };
}

vec3d vec3d::make(double xyz) {
    return vec3d{ xyz, xyz, xyz };
}

vec3d vec3d::make(const vec3i &v) {
    return vec3d{ (double)v.x, (double)v.y, (double)v.z };
}

vec3d vec3d::make(const struct vec3 &v) {
    return vec3d{ (double)v.x, (double)v.y, (double)v.z };
}

vec3d vec3d::absolute(const vec3d &vec) {
    return vec3d{ ABS(vec.x), ABS(vec.y), ABS(vec.z) };
}

double vec3d::length_sq(const vec3d &vec) {
    return vec3d::dot(vec, vec);
}

double vec3d::length(const vec3d &vec) {
    return sqrt(vec3d::length_sq(vec));
}

vec3d vec3d::normalize(const vec3d &vec) {
    const double length = vec3d::length(vec);
    return vec / length;
}

/* TODO Copied, learn */
vec3d vec3d::cross(const vec3d &a, const vec3d &b) {
    return vec3d{
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

double vec3d::dot(const vec3d &a, const vec3d &b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3d operator + (const vec3d &l, const vec3d r) {
    return vec3d{ l.x + r.x, l.y + r.y, l.z + r.z };
}

vec3d operator - (const vec3d &l, const vec3d r) {
    return vec3d{ l.x - r.x, l.y - r.y, l.z - r.z };
}

vec3d operator * (const vec3d &l, const vec3d r) {
    return vec3d{ l.x * r.x, l.y * r.y, l.z * r.z };
}

vec3d operator * (const vec3d &l, const double r) {
    return vec3d{ l.x * r, l.y * r, l.z * r };
}

vec3d operator * (const double l, const vec3d &r) {
    return r * l;
}

vec3d operator / (const vec3d &l, const double r) {
    return vec3d{ l.x / r, l.y / r, l.z / r };
}

vec3d &operator += (vec3d &l, const vec3d &r) {
    l = l + r;
    return l;
}

vec3d &operator -= (vec3d &l, const vec3d &r) {
    l = l - r;
    return l;
}

vec3d &operator *= (vec3d &l, const vec3d &r) {
    l = l * r;
    return l;
}

vec3d &operator *= (vec3d &l, const double &r) {
    l = l * r;
    return l;
}


/*
    --- vec3i ---
*/

vec3i vec3i::make(const vec3 &v) {
    return vec3i{ (int32_t)v.x, (int32_t)v.y, (int32_t)v.z };
}

bool operator == (const vec3i &l, const vec3i &r) {
    return l.x == r.x && l.y == r.y && l.z == r.z;
}

bool operator != (const vec3i &l, const vec3i &r) {
    return !(l == r);
}

vec3i operator + (const vec3i &l, const vec3i r) {
    return vec3i{ l.x + r.x, l.y + r.y, l.z + r.z };
}

vec3i operator - (const vec3i &l, const vec3i r) {
    return vec3i{ l.x - r.x, l.y - r.y, l.z - r.z };
}

/*
    --- mat4 ---
*/

mat4 mat4::transpose(const mat4 &m) {
    return mat4{
        m.e00, m.e10, m.e20, m.e30,
        m.e01, m.e11, m.e21, m.e31,
        m.e02, m.e12, m.e22, m.e32,
        m.e03, m.e13, m.e23, m.e33
    };
}

mat4 mat4::identity(void) {
    return mat4{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

/* TODO Copied from other project - Re-check this */
mat4 mat4::orthographic(float left, float bottom, float right, float top, float near, float far) {
    mat4 proj = mat4::identity();
    proj.e00 = 2.0f / (right - left);
    proj.e11 = 2.0f / (top - bottom);
    proj.e22 = 2.0f / (far - near);
    proj.e30 = -((right + left) / (right - left));
    proj.e31 = -((top + bottom) / (top - bottom));
    proj.e32 = -((far + near) / (far - near));
    return proj;
}

// FROM LINMATH
mat4 mat4::perspective(float fov, float aspect, float near, float far) {
    /* NOTE: Degrees are an unhandy unit to work with.
     * linmath.h uses radians for everything! */
    float const a = 1.f / tanf(fov / 2.f);

    mat4 m = mat4::identity();

    m.e00 = a / aspect;
    m.e01 = 0.f;
    m.e02 = 0.f;
    m.e03 = 0.f;

    m.e10 = 0.f;
    m.e11 = a;
    m.e12 = 0.f;
    m.e13 = 0.f;

    m.e20 = 0.f;
    m.e21 = 0.f;
    m.e22 = -((far + near) / (far - near));
    m.e23 = -1.f;

    m.e30 = 0.f;
    m.e31 = 0.f;
    m.e32 = -((2.f * far * near) / (far - near));
    m.e33 = 0.f;

    return m;
}

mat4 mat4::look_at(vec3 eye, vec3 focus, vec3 up_vec) {
    const vec3 z_axis = vec3::normalize(eye - focus);
    const vec3 x_axis = vec3::normalize(vec3::cross(up_vec, z_axis));
    const vec3 y_axis = vec3::cross(z_axis, x_axis);

   return mat4{
       x_axis.x, y_axis.x, z_axis.x, 0.0f,
       x_axis.y, y_axis.y, z_axis.y, 0.0f,
       x_axis.z, y_axis.z, z_axis.z, 0.0f,
       -vec3::dot(x_axis, eye), -vec3::dot(y_axis, eye), -vec3::dot(z_axis, eye), 1.0f,
    };
}

mat4 mat4::translate(const vec3 &vec) {
    return translate(vec.x, vec.y, vec.z);
}

mat4 mat4::translate(float x, float y, float z) {
    return {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        x, y, z, 1
    };
}

mat4 mat4::rotate_x(float theta) {
    const float s = sinf(theta);
    const float c = cosf(theta);
    return {
        1, 0, 0, 0,
        0, c,-s, 0,
        0, s, c, 0,
        0, 0, 0, 1
    };
}

mat4 mat4::rotate_y(float theta) {
    const float s = sinf(theta);
    const float c = cosf(theta);
    mat4 m = {
        c, 0, s, 0,
        0, 1, 0, 0,
       -s, 0, c, 0,
        0, 0, 0, 1
    };
    return m;

}

mat4 mat4::rotate_z(float theta) {
    const float s = sinf(theta);
    const float c = cosf(theta);
    mat4 m = {
        c,-s, 0, 0,
        s, c, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    return m;
}

mat4 mat4::scale(const vec3 &vec) {
    return scale(vec.x, vec.y, vec.z);
}

mat4 mat4::scale(float x, float y, float z) {
    return {
        x, 0, 0, 0,
        0, y, 0, 0,
        0, 0, z, 0,
        0, 0, 0, 1,
    };
}

mat4 operator * (const mat4 &l, const mat4 &r) {
    mat4 result = { };
    for(int32_t row = 0; row < 4; ++row) {
        for(int32_t col = 0; col < 4; ++col) {
            for(int32_t i = 0; i < 4; ++i) {
                result.e[row][col] += l.e[row][i] * r.e[i][col];
            }
        }
    }
    return result;
}

mat4 &operator *= (mat4 &l, const mat4 &r) {
    l = l * r;
    return l;
}

mat4 mat4::orthographic(double left, double bottom, double right, double top, double near, double far) {
    return mat4::orthographic((float)left, (float)bottom, (float)right, (float)top, (float)near, (float)far);
}

mat4 mat4::perspective(double fov, double aspect, double near, double far) {
    return mat4::perspective((float)fov, (float)aspect, (float)near, (float)far);
}

mat4 mat4::look_at(vec3d eye, vec3d focus, vec3d up_vec) {
    return mat4::look_at(vec3::make(eye), vec3::make(focus), vec3::make(up_vec));
}

mat4 mat4::translate(const vec3d &vec) {
    return mat4::translate(vec3::make(vec));
}

mat4 mat4::translate(double x, double y, double z) {
    return mat4::translate((float)x, (float)y, (float)z);
}

mat4 mat4::rotate_x(double theta) {
    return mat4::rotate_x((float)theta);
}

mat4 mat4::rotate_y(double theta) {
    return mat4::rotate_y((float)theta);
}

mat4 mat4::rotate_z(double theta) {
    return mat4::rotate_z((float)theta);
}

mat4 mat4::scale(const vec3d &vec) {
    return mat4::scale(vec3::make(vec));
}

mat4 mat4::scale(double x, double y, double z) {
    return mat4::scale((float)x, (float)y, (float)z);
}
