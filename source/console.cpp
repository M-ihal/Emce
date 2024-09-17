#include "console.h"

#include "shader.h"
#include "vertex_array.h"
#include "text_batcher.h"

#include <string>

// @temp
#include <glew.h>

#define CONSOLE_HISTORY_MAX 8
#define CONSOLE_INPUT_BUFFER_MAX 64

namespace {
    bool        s_initialized = false;
    TextBatcher s_batcher;
    std::string s_history[CONSOLE_HISTORY_MAX];
    char s_input_buffer[CONSOLE_INPUT_BUFFER_MAX];

    Font s_font;

    // @todo Make QuadBatcher or something
    ShaderFile  s_quad_shader;
    VertexArray s_quad_vao;
}


void Console::initialize(void) {
    ASSERT(!s_initialized);
    s_initialized = true;

    s_font.load_from_ttf_file("C://dev//emce//data//MinecraftRegular-Bmg3.otf", 30);
    // s_font.load_from_ttf_file("C://dev//emce//data//CascadiaMono.ttf", 24);

    s_quad_shader.set_filepath_and_load("C://dev//emce//source//shaders//simple_quad.glsl");

    BufferLayout layout;
    layout.push_attribute("a_position", 2, BufferDataType::FLOAT);

    const float vbo_data[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };

    const uint32_t ibo_data[] = { 0, 1, 2, 2, 3, 0 };

    s_quad_vao.create_vao(layout, ArrayBufferUsage::STATIC);
    s_quad_vao.set_ibo_data(ibo_data, ARRAY_COUNT(ibo_data));
    s_quad_vao.set_vbo_data(vbo_data, ARRAY_COUNT(vbo_data) * sizeof(float));
    s_quad_vao.apply_vao_attributes();
}

void Console::destroy(void) {
    s_initialized = false;

    s_quad_shader.delete_shader_file();
    s_quad_vao.delete_vao();
}

void Console::update(const Input &input) {
    s_quad_shader.hotload();
}

void Console::render(int32_t window_w, int32_t window_h) {
    const vec2 console_pos = { 22.0f, 20.0f };
    const vec2 console_text_padding = { 4.0f, 4.0f };

    mat4 model_m = mat4::identity();
    model_m *= mat4::scale(280.0f, 35.0f, 1.0f);
    model_m *= mat4::translate(console_pos.x, console_pos.y, 0.0f);
    
    /* Render input quad */
    s_quad_shader.use_program();
    s_quad_shader.upload_mat4("u_proj", mat4::orthographic(0.0f, 0.0f, float(window_w), float(window_h), -1.0f, 1.0f).e);
    s_quad_shader.upload_mat4("u_model", model_m.e);
    s_quad_shader.upload_vec4("u_color", vec4{ 0.0f, 0.0f, 0.0f, 0.4f }.e);

    s_quad_vao.bind_vao();
    GL_CHECK(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL));

    s_batcher.begin();
    s_batcher.push_text_formatted(console_pos + console_text_padding, s_font, "w: %d, h: %d", window_w, window_h);
    s_batcher.render(window_w, window_h, s_font);
}
