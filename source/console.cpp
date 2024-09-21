#include "console.h"

#include "shader.h"
#include "vertex_array.h"
#include "text_batcher.h"

#include <glew.h>
#include <SDL.h> // Start/StopTextInput
#include <string_view.h>

#define CONSOLE_HISTORY_MAX 12

namespace {
    bool        s_initialized = false;
    Font        s_font;
    TextBatcher s_batcher;
    ShaderFile  s_quad_shader;
    VertexArray s_quad_vao;

    bool        s_is_open = false;
    std::string s_history[CONSOLE_HISTORY_MAX];
    std::string s_input_buffer;
    std::string s_last_valid_input;

    std::vector<ConsoleCommand> s_commands;
}

void Console::initialize(void) {
    ASSERT(!s_initialized);
    s_initialized = true;

    /* Load font */
    s_font.load_from_file("C://dev//emce//data//MinecraftRegular-Bmg3.otf", 20);
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
    s_last_valid_input.clear();
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

void Console::update(const Input &input, Window &window, Game &game) {
    if(!s_is_open) {
        return;
    }

    /* Exit console */
    if(input.key_pressed(Key::ESCAPE)) {
        Console::set_open_state(false);
        return;
    }

    /* Get last valid input */
    if(input.key_pressed(Key::UP)) {
        s_input_buffer = s_last_valid_input;
    }

    /* Query text input */
    if(strlen(input.get_text_input())) {
        s_input_buffer += input.get_text_input();
    }

    /* Delete text */
    if(input.key_pressed_or_repeat(Key::BACKSPACE) && s_input_buffer.length()) {
        s_input_buffer.pop_back();
    }

    /* Apply command @todo Hacky */
    string_view_t command_view = string_view((char *)s_input_buffer.c_str());
    command_view = s_view_trim(command_view);
    if(input.key_pressed(Key::ENTER) && s_input_buffer.length()) {
        Console::add_to_history(s_input_buffer.c_str());

        // @temp Parsing Command and args
        string_view_t cmd;
        string_view_t arg1 = { };
        string_view_t arg2 = { };
        string_view_t arg3 = { };
        size_t space_idx = s_view_find_first(command_view, ' ');
        if(space_idx != s_view_invalid_idx) {
            string_view_t args;
            s_view_split(command_view, space_idx, &cmd, &args);
            args = s_view_trim(args);
            space_idx = s_view_find_first(args, ' ');
            if(space_idx != s_view_invalid_idx) {
                s_view_split(args, space_idx, &arg1, &args);
                args = s_view_trim(args);
                space_idx = s_view_find_first(args, ' ');
                if(space_idx != s_view_invalid_idx) {
                    s_view_split(args, space_idx, &arg2, &args);
                    args = s_view_trim(args);
                    space_idx = s_view_find_first(args, ' ');
                    if(space_idx != s_view_invalid_idx) {
                        s_view_split(args, space_idx, &arg3, &args);
                    } else {
                        arg3 = args;
                    }
                } else {
                    arg2 = args;
                }
            } else {
                arg1 = args;
            }
        } else {
            cmd = command_view;
        }

        for(auto &command : s_commands) {
            if(cmd == string_view((char *)command.command.c_str())) {
                s_last_valid_input = s_input_buffer;

                std::vector<std::string> args;
                if(arg1.length) { std::string s; s.assign(arg1.pointer, arg1.length); args.push_back(s); }
                if(arg2.length) { std::string s; s.assign(arg2.pointer, arg2.length); args.push_back(s); }
                if(arg3.length) { std::string s; s.assign(arg3.pointer, arg3.length); args.push_back(s); }
                command.proc(args, window, game);
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

    const vec2 console_text_padding = { 6.0f, 6.0f };
    const vec2 console_pos = { 32.0f, 32.0f };
    const vec2 console_size = { 480.0f, float(s_font.get_height() + console_text_padding.y) };

    /* Quad shader */
    s_quad_shader.use_program();
    s_quad_shader.upload_mat4("u_proj", mat4::orthographic(0.0f, 0.0f, float(window_w), float(window_h), -1.0f, 1.0f).e);
    s_quad_vao.bind_vao();

    s_batcher.begin();

    /* Blend for quads, disabled before rendering text batch */
    GL_CHECK(glEnable(GL_BLEND));

    /* Draw console input region */ {
        mat4 model_m = mat4::identity();
        model_m *= mat4::scale(console_size.x, console_size.y, 1.0f);
        model_m *= mat4::translate(console_pos.x, console_pos.y, 0.0f);

        s_quad_shader.upload_vec4("u_color", vec4{ 0.0f, 0.0f, 0.0f, 0.65f }.e);
        s_quad_shader.upload_mat4("u_model", model_m.e);

        GL_CHECK(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL));

        s_batcher.push_text_formatted(console_pos + console_text_padding, s_font, s_input_buffer.c_str());
    }

    /* Draw console history region */ {
        const vec2 history_pos = console_pos + vec2{ 0.0f, console_size.y };
        const vec2 history_size = { console_size.x, float(CONSOLE_HISTORY_MAX * s_font.get_height() + console_text_padding.y) };
        const float history_text_x = console_pos.x + console_text_padding.x;
        float history_text_y = history_pos.y + console_text_padding.y;

        mat4 model_m = mat4::identity();
        model_m *= mat4::scale(history_size.x, history_size.y, 1.0f);
        model_m *= mat4::translate(history_pos.x, history_pos.y, 0.0f);

        s_quad_shader.upload_vec4("u_color", vec4{ 0.0f, 0.0f, 0.0f, 0.45f }.e);
        s_quad_shader.upload_mat4("u_model", model_m.e);

        GL_CHECK(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL));

        for(int32_t history_index = 0; history_index < CONSOLE_HISTORY_MAX; ++history_index) {
            s_batcher.push_text({ history_text_x, history_text_y }, s_font, s_history[history_index].c_str());
            history_text_y += s_font.get_height();
        }
    }

    GL_CHECK(glDisable(GL_BLEND));
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

// @todo Check on command name and stuff?
void Console::register_command(ConsoleCommand command) {
    s_commands.push_back(command);
}
