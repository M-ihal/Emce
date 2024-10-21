#include "text_batcher.h"

#include "vertex_array.h"
#include "shader.h"

#include <malloc.h>
#include <stdio.h>
#include <cstdarg>
#include <cstring>

#include <glew.h>

namespace {
    bool        g_initialized = false;
    VertexArray g_vao;
    ShaderFile  g_shader;
};

void TextBatcher::initialize(void) {
    ASSERT(!g_initialized, "TextBatcher already initialized.\n");
    g_initialized = true;

    g_shader.set_filepath_and_load("C://dev//emce//source//shaders//simple_text.glsl");

    BufferLayout layout;
    layout.push_attribute("a_position", 2, BufferDataType::FLOAT);
    layout.push_attribute("a_tex_coord", 2, BufferDataType::FLOAT);
    g_vao.create_vao(layout, ArrayBufferUsage::DYNAMIC);
    g_vao.apply_vao_attributes();
        
    /* Allocate vertex memory in the vao */
    g_vao.set_vbo_data(NULL, TEXT_BATCHER_QUAD_MAX * 4 * sizeof(TextQuadVertex));

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
    g_vao.set_ibo_data(indices, TEXT_BATCHER_QUAD_MAX * 6);
    free(indices);
}

void TextBatcher::destroy(void) {
    g_vao.delete_vao();
    g_shader.delete_shader_file();
}

void TextBatcher::hotload_shader(void) {
    g_shader.hotload();
}

TextBatcher::TextBatcher(void) {
    const size_t size_in_bytes = TEXT_BATCHER_QUAD_MAX * 4 * sizeof(TextQuadVertex);
    m_vertices = (TextQuadVertex *)malloc(size_in_bytes);
    m_chars_pushed = 0;
    ASSERT(m_vertices);
    fprintf(stdout, "[info] TextBatcher: Allocated memory. %lld bytes.\n", size_in_bytes);
}

TextBatcher::~TextBatcher(void) {
    free(m_vertices);
    m_vertices = NULL;
    fprintf(stdout, "[info] TextBatcher: Deallocated memory.\n");
}

void TextBatcher::begin(void) {
    m_chars_pushed = 0;
}

void TextBatcher::render(int32_t width, int32_t height, const Font &font, vec3 color, vec2i shadow_offset) {
    // @todo Get rid of these somehow
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    g_shader.use_program();
    g_shader.upload_mat4("u_proj", mat4::orthographic(0.0f, 0.0f, float(width), float(height), -1.0f, 1.0f).e);
    g_shader.upload_int("u_font_atlas", 0);
    font.get_atlas().bind_texture_unit(0);

    g_vao.bind_vao();
    g_vao.upload_vbo_data(m_vertices, m_chars_pushed * 4 * sizeof(TextQuadVertex), 0);

    /* Draw text shadow */
    if(shadow_offset.x != 0 || shadow_offset.y != 0) {
        g_shader.upload_vec2("u_offset", vec2::make(shadow_offset).e);
        g_shader.upload_vec3("u_color", vec3{ 0.1f, 0.1f, 0.1f }.e);
        GL_CHECK(glDrawElements(GL_TRIANGLES, m_chars_pushed * 6, GL_UNSIGNED_INT, NULL));
    }

    /* Draw actual text */
    g_shader.upload_vec2("u_offset", vec2{ 0.0f, 0.0f }.e);
    g_shader.upload_vec3("u_color", color.e);
    GL_CHECK(glDrawElements(GL_TRIANGLES, m_chars_pushed * 6, GL_UNSIGNED_INT, NULL));

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

            ASSERT(m_chars_pushed + 1 <= TEXT_BATCHER_QUAD_MAX, "TextBatcher::push_text char limit exceeded.\n");

            TextQuadVertex *begintest = m_vertices;
            TextQuadVertex *next_vertex = m_vertices + m_chars_pushed * 4;
            *next_vertex++ = { vec2{ glyph_pos.x,                glyph_pos.y },                vec2{ glyph.tex_coords[0].x, glyph.tex_coords[0].y } };
            *next_vertex++ = { vec2{ glyph_pos.x + glyph_size.x, glyph_pos.y },                vec2{ glyph.tex_coords[1].x, glyph.tex_coords[1].y } };
            *next_vertex++ = { vec2{ glyph_pos.x + glyph_size.x, glyph_pos.y + glyph_size.y }, vec2{ glyph.tex_coords[2].x, glyph.tex_coords[2].y } };
            *next_vertex++ = { vec2{ glyph_pos.x,                glyph_pos.y + glyph_size.y }, vec2{ glyph.tex_coords[3].x, glyph.tex_coords[3].y } };
            m_chars_pushed += 1;
        }

        const int32_t adv = glyph.advance + font.get_kerning_advance(_char, string[index + 1]);
        text_cursor.x += float(adv) * font.get_scale_for_pixel_height();
    }
}


void TextBatcher::push_text_formatted(vec2 position, const Font &font, const char *format, ...) {
    va_list args;
    va_start(args, format);
    this->push_text_va_args(position, font, format, args);
    va_end(args);
}

void TextBatcher::push_text_va_args(vec2 position, const Font &font, const char *format, va_list args) {
    /* @todo Measure required space and allocate? For now statically alloc 1024 chars... */
    char buffer[1024];
    vsnprintf_s(buffer, ARRAY_COUNT(buffer), format, args);
    push_text(position, font, buffer);
}
