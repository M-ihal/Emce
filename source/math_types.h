#pragma once

#if !defined(M_PI)
#define M_PI 3.14159265f
#endif

#define DEG_TO_RAD(deg) ((deg) * (M_PI / 180.0f))
#define RAD_TO_DEG(rad) ((rad) * (180.0f / M_PI))

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
    static vec2 zero(void);
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
    static vec3 zero(void);

    static float length_sq(const vec3 &vec);
    static float length(const vec3 &vec);
    static vec3  normalize(const vec3 &vec);
    static vec3  cross(const vec3 &a, const vec3 &b);
    static float dot(const vec3 &a, const vec3 &b);
};

vec3 operator + (const vec3 &l, const vec3 r);
vec3 operator - (const vec3 &l, const vec3 r);
vec3 operator / (const vec3 &l, const float r);

struct mat4 {
    union {
        struct {
            float e00, e10, e20, e30;
            float e01, e11, e21, e31;
            float e02, e12, e22, e32;
            float e03, e13, e23, e33;
        };
        float e[4][4];
    };

    static mat4 identity(void);
    static mat4 orthographic(float left, float bottom, float right, float top, float near, float far);
    static mat4 perspective(float fov, float aspect, float near, float far);
    static mat4 look_at(vec3 eye, vec3 focus, vec3 up_vec);
    static mat4 translate(const vec3 &vec);
};
