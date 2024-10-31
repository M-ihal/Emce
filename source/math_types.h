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

template <typename TVal, typename TPerc> TVal lerp(TVal a, TVal b, TPerc t);

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
    static vec2 make_xz(const struct vec3 &v);
    static vec2 zero(void);

    static vec2  absolute(const vec2 &v);
    static float length_sq(const vec2 &v);
    static float length(const vec2 &v);
    static float dot(const vec2 &a, const vec2 &b);
    static vec2  normalize(const vec2 &v);
};

vec2 operator + (const vec2 &l, const vec2 &r);
vec2 operator - (const vec2 &l, const vec2 &r);
vec2 operator * (const vec2 &l, float r);
vec2 &operator *= (vec2 &l, float r);

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

    vec2 get_xz(void) const;

    static vec3 zero(void);
    static vec3 make(float x, float y, float z);
    static vec3 make(float xyz);
    static vec3 make(const vec2 &v, float z);
    static vec3 make(const struct vec3i &v);

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

vec3i operator + (const vec3i &l, const vec3i r);
vec3i operator - (const vec3i &l, const vec3i r);

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

/* Row major */
struct mat4 {
    union {
        float e[4][4];
        struct {
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
    static mat4 rotate_x(float theta);
    static mat4 rotate_y(float theta);
    static mat4 rotate_z(float theta);
    static mat4 scale(const vec3 &vec);
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

template <typename TVal, typename TPerc>
TVal lerp(TVal a, TVal b, TPerc t) {
    TVal res = t * b + (1 - t) * a;
    return res;
}
