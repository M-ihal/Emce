#pragma once

#include "common.h"

#if !defined(M_PI)
#define M_PI 3.14159265f
#endif

#define DEG_TO_RAD(deg) ((deg) * (M_PI / 180.0f))
#define RAD_TO_DEG(rad) ((rad) * (180.0f / M_PI))
#define SQUARE(v) ((v) * (v))

#define SIGN(v) (((v) < 0) ? -1 : 1)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define ABS(v) ((v) < 0 ? -(v) : (v))

float wrap(float value, float left, float right);

template <typename T> T    clamp_min(const T v, const T min);
template <typename T> T    clamp_max(const T v, const T max);
template <typename T> T    clamp(const T v, const T min, const T max);
template <typename T> void clamp_min_v(T &v, const T min);
template <typename T> void clamp_max_v(T &v, const T max);
template <typename T> void clamp_v(T &v, const T min, const T max);

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

    static float length_sq(const vec2 &v);
    static float length(const vec2 &v);
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
    static vec2i absolute(const vec2i &v);
};

vec2i operator + (const vec2i &l, const vec2i &r);
vec2i operator - (const vec2i &l, const vec2i &r);
vec2i operator / (const vec2i &l, int32_t r);
bool operator == (const vec2i &l, const vec2i &r);
bool operator != (const vec2i &l, const vec2i &r);

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

vec3 operator + (const vec3 &l, const vec3 r);
vec3 operator - (const vec3 &l, const vec3 r);
vec3 operator * (const vec3 &l, const vec3 r);
vec3 operator * (const vec3 &l, const float r);
vec3 operator * (const float l, const vec3 &r);
vec3 operator / (const vec3 &l, const float r);
vec3 &operator += (vec3 &l, const vec3 &r);
vec3 &operator -= (vec3 &l, const vec3 &r);
vec3 &operator *= (vec3 &l, const vec3 &r);
vec3 &operator *= (vec3 &l, const float &r);


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

template <typename T>
T clamp_min(const T v, const T min) {
    if(v < min) {
        return min;
    } else {
        return v;
    }
}

template <typename T>
T clamp_max(const T v, const T max) {
    if(v > max) {
        return max;
    } else {
        return v;
    }
}

template <typename T>
T clamp(const T v, const T min, const T max) {
    ASSERT(min <= max);

    if(v < min) {
        return min;
    } else if(v > max) {
        return max;
    } else {
        return v;
    }
}

template <typename T>
void clamp_min_v(T &v, const T min) {
    if(v < min) {
        v = min;
    }
}

template <typename T>
void clamp_max_v(T &v, const T max) {
    if(v > max) {
        v = max;
    }
}

template <typename T>
void clamp_v(T &v, const T min, const T max) {
    ASSERT(min <= max);
 
    if(v < min) {
        v = min;
    } else if(v > max) {
        v = max;
    }
}
