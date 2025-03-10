#pragma once

#include "common.h"
#include "math_types.h"

enum class BlendFunc {
    DISABLE = 0,
    STANDARD
};

enum class DepthFunc {
    DISABLE = 0,
    LESS,
    LESS_OR_EQUAL,
    ALWAYS
};

struct RenderSetup {
    BlendFunc blend = BlendFunc::DISABLE;
    DepthFunc depth = DepthFunc::DISABLE;
    bool multisample = true;
    bool cull_faces = false;
    bool disable_depth_write = false;
};

void set_viewport(const vec4i &viewport);
void set_polygon_mode(bool enable);
void set_render_state(const RenderSetup &setup);
void set_line_width(float width);

/* Assumes GL_UNSIGNED_INT as indices type */
void draw_elements_triangles(uint32_t index_count, uint64_t offset = 0);
void draw_elements_lines(uint32_t index_count, uint64_t offset = 0);

