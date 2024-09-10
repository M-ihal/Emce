#pragma once

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
};
