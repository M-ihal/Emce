#pragma once

#include "common.h"
#include "math_types.h"
#include "camera.h"

/*
    Collection of simple drawing shapes like cube outlines and stuff
*/

/* @todo Move */
struct Color {
    union {
        struct {
            float r;
            float g;
            float b;
            float a;
        };
        vec4  v;
        float e[4];
    };

    static Color from_255(const vec4 &c255) {
        return {
            c255.r / 255.0f,
            c255.g / 255.0f,
            c255.b / 255.0f,
            c255.a / 255.0f,
        };
    }

    static Color from_hex(uint32_t hex) {
        union _Hex {
            uint32_t hex;
            uint8_t  hex_[4];
        } _hex;

        _hex.hex = hex;

        return {
            (float)_hex.hex_[0] / 255.0f,
            (float)_hex.hex_[1] / 255.0f,
            (float)_hex.hex_[2] / 255.0f,
            (float)_hex.hex_[3] / 255.0f,
        };
    }
};

namespace SimpleDraw {
    void initialize(void);
    void destroy(void);
    void hotload_shader(void);
    void set_camera(const Camera &camera, float aspect_ratio);
    void draw_cube_outline(const vec3 &position, const vec3 &size, float width, const Color &color);
}