#include "text_batcher.h"

#include "vertex_array.h"
#include "shader.h"

#include <malloc.h>
#include <stdio.h>
#include <cstdarg>

#include <glew.h>

struct TextQuadVertex {
    vec2 position;
    vec2 tex_coord;
};

namespace {
    bool        s_initialized = false;
    VertexArray s_vao;
    ShaderFile  s_shader;

    TextQuadVertex *s_vertices;
    uint32_t        s_chars_pushed;
};

void TextBatcher::initialize(void) {
    ASSERT(!s_initialized, "TextBatcher already initialized.\n");
    s_initialized = true;

    s_shader.set_filepath_and_load("C://dev//emce//source//shaders//simple_text.glsl");

    BufferLayout layout;
    layout.push_attribute("a_position", 2, BufferDataType::FLOAT);
    layout.push_attribute("a_tex_coord", 2, BufferDataType::FLOAT);
    s_vao.create_vao(layout, ArrayBufferUsage::DYNAMIC);
    s_vao.apply_vao_attributes();
        
    /* Allocate vertex memory */
    s_vertices = (TextQuadVertex *)malloc(TEXT_BATCHER_QUAD_MAX * 4 * sizeof(TextQuadVertex));
    s_vao.set_vbo_data(NULL, TEXT_BATCHER_QUAD_MAX * 4 * sizeof(TextQuadVertex));

    /* Set vao indices, those don't change ever */
    uint32_t *indices = (uint32_t *)malloc(TEXT_BATCHER_QUAD_MAX * 6 * sizeof(uint32_t));
    for(int32_t quad_index = 0; quad_index < TEXT_BATCHER_QUAD_MAX; ++quad_index) {
        uint32_t *cursor = indices + quad_index * 6;
        uint32_t begin_index = quad_index * 4;
        *cursor++ = begin_index + 0;
        *cursor++ = begin_index + 1;
        *cursor++ = begin_index + 2;
        *cursor++ = begin_index + 2;
        *cursor++ = begin_index + 3;
        *cursor++ = begin_index + 0;
    }
    s_vao.set_ibo_data(indices, TEXT_BATCHER_QUAD_MAX * 6);
    free(indices);
}

void TextBatcher::destroy(void) {
    s_vao.delete_vao();
    s_shader.delete_shader_file();
}

void TextBatcher::begin(void) {
    /* For now try to hotload shader every begin() call */
    s_shader.hotload();
    s_chars_pushed = 0;
}

void TextBatcher::render(int32_t width, int32_t height, const Font &font, vec2i shadow_offset) {
    // @todo Get rid of these somehow
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    s_shader.use_program();
    s_shader.upload_mat4("u_proj", mat4::orthographic(0.0f, 0.0f, float(width), float(height), -1.0f, 1.0f).e);
    s_shader.upload_int("u_font_atlas", 0);
    font.get_atlas().bind_texture_unit(0);

    s_vao.bind_vao();
    s_vao.upload_vbo_data(s_vertices, s_chars_pushed * 4 * sizeof(TextQuadVertex), 0);

    /* Draw text shadow */
    if(shadow_offset.x != 0 || shadow_offset.y != 0) {
        s_shader.upload_vec2("u_offset", vec2::make(shadow_offset).e);
        s_shader.upload_vec3("u_color", vec3{ 0.0f, 0.0f, 0.0f }.e);
        GL_CHECK(glDrawElements(GL_TRIANGLES, s_chars_pushed * 6, GL_UNSIGNED_INT, NULL));
    }

    /* Draw actual text */
    s_shader.upload_vec2("u_offset", vec2{ 0.0f, 0.0f }.e);
    s_shader.upload_vec3("u_color", vec3{ 1.0f, 1.0f, 1.0f }.e);
    GL_CHECK(glDrawElements(GL_TRIANGLES, s_chars_pushed * 6, GL_UNSIGNED_INT, NULL));


    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void TextBatcher::push_text(vec2 position, const Font &font, const char *string) {
    const size_t length = strlen(string);

    vec2 text_cursor = position;
    for(int32_t index = 0; index < length; ++index) {
        const char _char = string[index];
        Font::Glyph glyph;

        /* @todo Special characters not handled... */
        switch(_char) {
            case '\n':
            case '\r':
            case '\t': {
                INVALID_CODE_PATH;
                continue;
            };
        }

        if(!font.get_glyph(_char, glyph)) {
            /* No glyph entry in the hash */
            continue;
        }

        if(glyph.has_glyph) {
            const vec2 glyph_size = { float(glyph.width), float(glyph.height) };
            const vec2 glyph_pos = {
                .x = text_cursor.x + glyph.left_side_bearing * font.get_scale_for_pixel_height(),
                .y = text_cursor.y - glyph_size.y - glyph.offset_y
            };

            ASSERT(s_chars_pushed + 1 <= TEXT_BATCHER_QUAD_MAX, "TextBatcher::push_text char limit exceeded.\n");

            TextQuadVertex *begintest = s_vertices;
            TextQuadVertex *next_vertex = s_vertices + s_chars_pushed * 4;
            *next_vertex++ = { vec2{ glyph_pos.x,                glyph_pos.y },                vec2{ glyph.tex_coords[0].x, glyph.tex_coords[0].y } };
            *next_vertex++ = { vec2{ glyph_pos.x + glyph_size.x, glyph_pos.y },                vec2{ glyph.tex_coords[1].x, glyph.tex_coords[1].y } };
            *next_vertex++ = { vec2{ glyph_pos.x + glyph_size.x, glyph_pos.y + glyph_size.y }, vec2{ glyph.tex_coords[2].x, glyph.tex_coords[2].y } };
            *next_vertex++ = { vec2{ glyph_pos.x,                glyph_pos.y + glyph_size.y }, vec2{ glyph.tex_coords[3].x, glyph.tex_coords[3].y } };
            s_chars_pushed += 1;
        }

        const int32_t adv = glyph.advance + font.get_kerning_advance(_char, string[index + 1]);
        text_cursor.x += float(adv) * font.get_scale_for_pixel_height();
    }
}


void TextBatcher::push_text_formatted(vec2 position, const Font &font, const char *format, ...) {
    va_list args;
    va_start(args, format);

    /* @todo Measure required space and allocate? For now statically alloc 1024 chars... */
    char buffer[1024];
    vsnprintf_s(buffer, ARRAY_COUNT(buffer), format, args);
    push_text(position, font, buffer);

    va_end(args);
}
