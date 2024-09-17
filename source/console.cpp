#include "console.h"

#include "shader.h"
#include "vertex_array.h"
#include "text_batcher.h"

#include <string>

// @temp
#include <glew.h>
#include <SDL.h> // Start/StopTextInput

#define CONSOLE_HISTORY_MAX 8
#define CONSOLE_INPUT_BUFFER_MAX 64

namespace {
    bool        s_initialized = false;
    TextBatcher s_batcher;
    std::string s_history[CONSOLE_HISTORY_MAX];

    std::string s_input_buffer;

    bool s_is_open = false;

    Font s_font;

    // @todo Make QuadBatcher or something
    ShaderFile  s_quad_shader;
    VertexArray s_quad_vao;
}


void Console::initialize(void) {
    ASSERT(!s_initialized);
    s_initialized = true;

    s_font.load_from_file("C://dev//emce//data//MinecraftRegular-Bmg3.otf", 30);
    // s_font.load_from_file("C://dev//emce//data//CascadiaMono.ttf", 24);
    s_font.get_atlas().set_filter_min(TextureFilter::NEAREST);
    s_font.get_atlas().set_filter_mag(TextureFilter::NEAREST);

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

    s_input_buffer.clear();
}

void Console::destroy(void) {
    s_initialized = false;

    s_quad_shader.delete_shader_file();
    s_quad_vao.delete_vao();
}

void Console::update(const Input &input) {
    if(!s_is_open) {
        return;
    }

    if(input.key_pressed(Key::ESCAPE)) {
        Console::set_open_state(false);
        return;
    }

    s_quad_shader.hotload();

    if(strlen(input.get_text_input())) {
        s_input_buffer += input.get_text_input();
    }

    if(input.key_pressed_or_repeat(Key::BACKSPACE) && s_input_buffer.length()) {
        s_input_buffer.pop_back();
    }

    if(input.key_pressed(Key::ENTER)) {
        // @temp
        if(s_input_buffer == "/close") {
            Console::set_open_state(false);
        }

        s_input_buffer.clear();
    }
}

void Console::render(int32_t window_w, int32_t window_h) {
    if(!s_is_open) {
        return;
    }

    const vec2 console_pos = { 32.0f, 32.0f };
    const vec2 console_text_padding = { 6.0f, 6.0f };

    mat4 model_m = mat4::identity();
    model_m *= mat4::scale(480.0f, 35.0f, 1.0f);
    model_m *= mat4::translate(console_pos.x, console_pos.y, 0.0f);
    
    /* Render input quad */
    s_quad_shader.use_program();
    s_quad_shader.upload_mat4("u_proj", mat4::orthographic(0.0f, 0.0f, float(window_w), float(window_h), -1.0f, 1.0f).e);
    s_quad_shader.upload_mat4("u_model", model_m.e);
    s_quad_shader.upload_vec4("u_color", vec4{ 0.0f, 0.0f, 0.0f, 0.65f }.e);
    s_quad_vao.bind_vao();

    GL_CHECK(glEnable(GL_BLEND));
    GL_CHECK(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL));
    GL_CHECK(glDisable(GL_BLEND));

    s_batcher.begin();
    s_batcher.push_text_formatted(console_pos + console_text_padding, s_font, s_input_buffer.c_str());
    s_batcher.render(window_w, window_h, s_font);
}

void Console::set_open_state(bool open) {
    if(s_is_open == open) {
        return;
    }

    if(open) {
        s_input_buffer.clear();
        SDL_StartTextInput();
    } else {
        SDL_StopTextInput();
    }

    s_is_open = open;
}

bool Console::is_open(void) {
    return s_is_open;
}

