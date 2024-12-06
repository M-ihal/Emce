#pragma once

#include "common.h"
#include "math_types.h"
#include "camera.h"

class Texture;
class TextureMultisample;

namespace SimpleDraw {
    void initialize(void);
    void destroy(void);
    void hotload_shader(void);
    void set_camera(const Camera &camera, double aspect_ratio);

    void draw_cube_outline(const vec3d &position, const vec3d &size, float width, const vec3 &color, float border_perc = 0.0, const vec3 &border_color = { 0.0f, 0.0f, 0.0f });
    void draw_cube_line_outline(const vec3d &position, const vec3d &size, const vec3 &color);
    void draw_line(const vec3d &point_a, const vec3d &point_b, float width, const vec3 &color);
}
