#pragma once

#include "common.h"

#if !defined(M_PI)
#define M_PI 3.14159265f
#endif

#define DEG_TO_RAD(deg) ((deg) * (M_PI / 180.0f))
#define RAD_TO_DEG(rad) ((rad) * (180.0f / M_PI))

#define SQUARE(v) ((v) * (v))

float clamp(float value, float min, float max);
float wrap(float value, float left, float right);

struct vec2 {
    union {
        struct {
            float x;
            float y;
        };
        float e[2];
    };

    static vec2 make(float x, float y);
    static vec2 make(float xy);
    static vec2 make(const struct vec2i &v);
    static vec2 zero(void);
};

vec2 operator + (const vec2 &l, const vec2 &r);

struct vec2i {
    union {
        struct {
            int32_t x;
            int32_t y;
        };
        int32_t e[2];
    };

    static vec2i zero(void);
};

struct vec3 {
    union {
        struct {
            float x;
            float y;
            float z;
        };
        struct {
            vec2 xy;
        };
        float e[3];
    };

    static vec3 make(float x, float y, float z);
    static vec3 make(float xyz);
    static vec3 make(const vec2 &v, float z);
    static vec3 make(const struct vec3i &v);
    static vec3 zero(void);

    static float length_sq(const vec3 &vec);
    static float length(const vec3 &vec);
    static vec3  normalize(const vec3 &vec);
    static vec3  cross(const vec3 &a, const vec3 &b);
    static float dot(const vec3 &a, const vec3 &b);
};

struct vec3i {
    union {
        struct {
            int32_t x;
            int32_t y;
            int32_t z;
        };
        int32_t e[3];
    };

    static vec3i make(const vec3 &v);
};

vec3 operator + (const vec3 &l, const vec3 r);
vec3 operator - (const vec3 &l, const vec3 r);
vec3 operator / (const vec3 &l, const float r);
vec3 &operator += (vec3 &l, const vec3 &r);

struct vec4 {
    union {
        struct {
            float x;
            float y;
            float z;
            float w;
        };
        struct {
            float r;
            float g;
            float b;
            float a;
        };
        float e[4];
    };
};

/* The matrix operations are column major, but initialization is row major due to memory layout */
/*
    i.e. Translation matrix would be initialized like:

    mat4 m = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        x, y, z, 1
    };
*/

struct mat4 {
    union {
        float e[4][4];
        struct { /* Memory layout of the matrix */
            float e00, e01, e02, e03;
            float e10, e11, e12, e13;
            float e20, e21, e22, e23;
            float e30, e31, e32, e33;
        };
    };

    static mat4 transpose(const mat4 &m);
    static mat4 identity(void);
    static mat4 orthographic(float left, float bottom, float right, float top, float near, float far);
    static mat4 perspective(float fov, float aspect, float near, float far);
    static mat4 look_at(vec3 eye, vec3 focus, vec3 up_vec);
    static mat4 translate(const vec3 &vec);
    static mat4 translate(float x, float y, float z);
    static mat4 rotate(float x, float y, float z);
    static mat4 scale(float x, float y, float z);
};

mat4 operator * (const mat4 &l, const mat4 &r);
mat4 &operator *= (mat4 &l, const mat4 &r);
