#pragma once

#include "common.h"

/* 
    Abstraction for opengl to avoid confusing gl* calls... 
*/

enum class BlendFunc {
    DISABLE,
    SRC_ALPHA__ONE_MINUS_SRC_ALPHA
};

enum class DepthFunc {
    DISABLE,
    LESS,
    LESS_OR_EQUAL
};

struct RenderSetup {
    BlendFunc blend = BlendFunc::DISABLE;
    DepthFunc depth = DepthFunc::DISABLE;
    bool multisample = true;
};

void set_render_state(const RenderSetup &setup);

/* Assumes GL_UNSIGNED_INT as indices type */

void draw_elements_triangles(uint32_t index_count, uint64_t offset = 0);

void draw_elements_lines(uint32_t index_count, uint64_t offset = 0);

/**/
void set_line_width(float width);
