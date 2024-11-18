#include "opengl_abs.h"

#include <glew.h>

void set_viewport(const vec4i &viewport) {
    GL_CHECK(glViewport(viewport.x, viewport.y, viewport.z, viewport.w));
}

void set_polygon_mode(bool enable) {
    if(enable) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void set_render_state(const RenderSetup &setup) {
    switch(setup.blend) {
        default: INVALID_CODE_PATH;

        case BlendFunc::DISABLE: {
            glDisable(GL_BLEND);
        } break;

        case BlendFunc::STANDARD: {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        } break;
    }

    switch(setup.depth) {
        default: INVALID_CODE_PATH;

        case DepthFunc::DISABLE: {
            glDisable(GL_DEPTH_TEST);
        } break;

        case DepthFunc::LESS: {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
        } break;

        case DepthFunc::LESS_OR_EQUAL: {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);
        } break;
    }

    if(setup.multisample) {
        glEnable(GL_MULTISAMPLE);
    } else {
        glDisable(GL_MULTISAMPLE);
    }

    if(setup.cull_faces) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
    } else {
        glDisable(GL_CULL_FACE);
    }
}

void draw_elements_triangles(uint32_t index_count, uint64_t offset) {
    GL_CHECK(glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, (void *)offset));
}

void draw_elements_lines(uint32_t index_count, uint64_t offset) {
    GL_CHECK(glDrawElements(GL_LINES, index_count, GL_UNSIGNED_INT, (void *)offset));
}

void set_line_width(float width) {
    GL_CHECK(glLineWidth(width));
}
