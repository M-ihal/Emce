#include "console.h"

#include "shader.h"
#include "vertex_array.h"
#include "text_batcher.h"

// @temp
#include <string>
#include <glew.h>
#include <SDL.h> // Start/StopTextInput

#define CONSOLE_HISTORY_MAX 8
#define CONSOLE_INPUT_BUFFER_MAX 64

namespace {
    bool        s_initialized = false;
    Font        s_font;
    TextBatcher s_batcher;
    ShaderFile  s_quad_shader;
    VertexArray s_quad_vao;

    bool        s_is_open = false;
    std::string s_history[CONSOLE_HISTORY_MAX];
    std::string s_input_buffer;

    std::vector<ConsoleCommand> s_commands;
}

void Console::initialize(void) {
    ASSERT(!s_initialized);
    s_initialized = true;

    /* Load font */
    s_font.load_from_file("C://dev//emce//data//MinecraftRegular-Bmg3.otf", 30);
    // s_font.load_from_file("C://dev//emce//data//CascadiaMono.ttf", 24);
    s_font.get_atlas().set_filter_min(TextureFilter::NEAREST);
    s_font.get_atlas().set_filter_mag(TextureFilter::NEAREST);

    /* Load the quad shader */
    s_quad_shader.set_filepath_and_load("C://dev//emce//source//shaders//simple_quad.glsl");

    /* Initialize quad vertex array */
    const float vbo_data[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };

    const uint32_t ibo_data[] = {
        0, 1, 2,
        2, 3, 0 
    };

    BufferLayout layout;
    layout.push_attribute("a_position", 2, BufferDataType::FLOAT);

    s_quad_vao.create_vao(layout, ArrayBufferUsage::STATIC);
    s_quad_vao.set_ibo_data(ibo_data, ARRAY_COUNT(ibo_data));
    s_quad_vao.set_vbo_data(vbo_data, ARRAY_COUNT(vbo_data) * sizeof(float));
    s_quad_vao.apply_vao_attributes();
    
    /* Initialize console */
    s_is_open = false;
    s_input_buffer.clear();
    s_commands.clear();
}

void Console::destroy(void) {
    s_initialized = false;

    s_font.delete_font();
    s_quad_shader.delete_shader_file();
    s_quad_vao.delete_vao();
}

void Console::add_to_history(const char *string) {
    /* Push history */
    for(int32_t index = CONSOLE_HISTORY_MAX - 1; index >= 1; --index) {
        s_history[index] = s_history[index - 1];
    }

    s_history[0] = std::string(string);
}

void Console::update(const Input &input, Window &window, Camera &camera) {
    if(!s_is_open) {
        return;
    }

    /* Exit console */
    if(input.key_pressed(Key::ESCAPE)) {
        Console::set_open_state(false);
        return;
    }

    /* Query text input */
    if(strlen(input.get_text_input())) {
        s_input_buffer += input.get_text_input();
    }

    /* Delete text */
    if(input.key_pressed_or_repeat(Key::BACKSPACE) && s_input_buffer.length()) {
        s_input_buffer.pop_back();
    }

    /* Apply command */
    if(input.key_pressed(Key::ENTER)) {
        for(auto &command : s_commands) {
            if(s_input_buffer == command.command) {
                Console::add_to_history(s_input_buffer.c_str());
                command.proc(window, camera);
                // Console::set_open_state(false);
                break;
            }
        }

        s_input_buffer.clear();
    }
}

void Console::render(int32_t window_w, int32_t window_h) {
    if(!s_is_open) {
        return;
    }

    s_quad_shader.hotload();

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

#if 1
    for(int32_t index = 0; index < CONSOLE_HISTORY_MAX; ++index) {
        s_batcher.push_text_formatted(console_pos + console_text_padding + vec2{ 0.0f, float(index + 1.0f) * float(s_font.get_height()) }, s_font, s_history[index].c_str());
    }
#endif

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

void Console::register_command(ConsoleCommand command) {
    s_commands.push_back(command);
}
