#pragma once

#include "common.h"
#include "math_types.h"
#include "camera.h"

/*
    Collection of simple drawing shapes like cube outlines and stuff
*/

class Texture;
class TextureMultisample;

namespace SimpleDraw {
    void initialize(void);
    void destroy(void);
    void hotload_shader(void);
    void set_camera(const Camera &camera, float aspect_ratio);
    void draw_triangle(const vec3 &tri_a, const vec3 &tri_b, const vec3 &tri_c, const vec4 &color);
    void draw_cube_outline(const vec3 &position, const vec3 &size, float width, const vec3 &color, float border_perc = 0.0f, const vec3 &border_color = { 0.0f, 0.0f, 0.0f });
    void draw_line(const vec3 &point_a, const vec3 &point_b, float width, const vec3 &color);
    void draw_textured_quad_2d(const vec2 &position, const vec2 &size, const Texture &texture, mat4 &proj_m, float z_pos = 0.0f);
}
