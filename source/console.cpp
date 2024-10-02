#include "console.h"

#include "shader.h"
#include "vertex_array.h"
#include "text_batcher.h"

#include <glew.h>
#include <SDL.h> // Start/StopTextInput
#include <string_view.h>

/* 
    @todo
    - pasting
    - moving through input buffer
    - scissor
    - max command length
*/

namespace {
    constexpr int32_t CONSOLE_HISTORY_MAX = 32;
    constexpr int32_t CONSOLE_INPUT_MAX   = 64;

    bool        g_initialized = false;
    Font        g_font;
    TextBatcher g_batcher;
    ShaderFile  g_quad_shader;
    VertexArray g_quad_vao;

    bool        g_is_open = false;
    std::string g_history[CONSOLE_HISTORY_MAX];
    std::string g_input_buffer;
    std::string g_last_valid_input;
    std::vector<ConsoleCommand> g_commands;

    constexpr double CURSOR_BLINK_TIME = 0.5;
    double g_cursor_blink_t = 0.0;
    bool   g_cursor_blink   = false;
}

void Console::initialize(void) {
    ASSERT(!g_initialized);
    g_initialized = true;

    /* Load font */
    g_font.load_from_file("C://dev//emce//data//MinecraftRegular-Bmg3.otf", 20);
    g_font.get_atlas().set_filter_min(TextureFilter::NEAREST);
    g_font.get_atlas().set_filter_mag(TextureFilter::NEAREST);

    /* Load the quad shader */
    g_quad_shader.set_filepath_and_load("C://dev//emce//source//shaders//simple_quad.glsl");

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

    g_quad_vao.create_vao(layout, ArrayBufferUsage::STATIC);
    g_quad_vao.set_ibo_data(ibo_data, ARRAY_COUNT(ibo_data));
    g_quad_vao.set_vbo_data(vbo_data, ARRAY_COUNT(vbo_data) * sizeof(float));
    g_quad_vao.apply_vao_attributes();
    
    /* Initialize console */
    g_is_open = false;
    g_input_buffer.clear();
    g_last_valid_input.clear();
    g_commands.clear();

    Console::register_command({
        .command = "commands",
        .proc = CONSOLE_COMMAND_LAMBDA {
            Console::add_to_history("> registered commands:");
            for(auto &command : g_commands) {
                Console::add_to_history(command.command.c_str());
            }
        }
    });

    Console::register_command({
        .command = "clear",
        .proc = CONSOLE_COMMAND_LAMBDA {
            for(int32_t index = 0; index < CONSOLE_HISTORY_MAX; ++index) {
                g_history[index].clear();
            }
        }
    });
}

void Console::destroy(void) {
    g_initialized = false;

    g_font.delete_font();
    g_quad_shader.delete_shader_file();
    g_quad_vao.delete_vao();
}

void Console::add_to_history(const char *string) {
    /* Push history */
    for(int32_t index = CONSOLE_HISTORY_MAX - 1; index >= 1; --index) {
        g_history[index] = g_history[index - 1];
    }

    g_history[0] = std::string(string);
}

void Console::update(const Input &input, Window &window, Game &game, double delta_time) {
    if(!g_is_open) {
        return;
    }

    /* If changed reset cursor blink */
    bool input_buffer_changed = false;

    /* Exit console */
    if(input.key_pressed(Key::ESCAPE)) {
        Console::set_open_state(false);
        return;
    }

    /* Get last valid input */
    if(input.key_pressed(Key::UP)) {
        g_input_buffer = g_last_valid_input;
        input_buffer_changed = true;
    }

    /* Query text input */
    size_t input_len = strlen(input.get_text_input());
    if(input_len) {
        for(int32_t index = 0; index < input_len; ++index) {
            if(g_input_buffer.length() >= CONSOLE_INPUT_MAX) {
                break;
            }
            g_input_buffer += input.get_text_input();
            input_buffer_changed = true;
        }
    }

    /* Delete text */
    if(input.key_pressed_or_repeat(Key::BACKSPACE) && g_input_buffer.length()) {
        g_input_buffer.pop_back();
        input_buffer_changed = true;
    }

    /* Apply command @todo Hacky */
    string_view_t command_view = string_view((char *)g_input_buffer.c_str());
    command_view = s_view_trim(command_view);
    if(input.key_pressed(Key::ENTER) && g_input_buffer.length()) {
        Console::add_to_history(g_input_buffer.c_str());

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

        for(auto &command : g_commands) {
            if(cmd == string_view((char *)command.command.c_str())) {
                g_last_valid_input = g_input_buffer;

                std::vector<std::string> args;
                if(arg1.length) { std::string s; s.assign(arg1.pointer, arg1.length); args.push_back(s); }
                if(arg2.length) { std::string s; s.assign(arg2.pointer, arg2.length); args.push_back(s); }
                if(arg3.length) { std::string s; s.assign(arg3.pointer, arg3.length); args.push_back(s); }
                command.proc(args, window, game);
                break;
            }
        }

        g_input_buffer.clear();
    }

    if(input_buffer_changed) {
        g_cursor_blink_t = 0.0;
        g_cursor_blink = false;
    } else {
        g_cursor_blink_t += delta_time;
        if(g_cursor_blink_t >= CURSOR_BLINK_TIME) {
            g_cursor_blink_t = 0.0;
            BOOL_TOGGLE(g_cursor_blink);
        }
    }
}

void Console::render(int32_t window_w, int32_t window_h) {
    if(!g_is_open) {
        return;
    }

    g_quad_shader.hotload();

    const vec2 console_text_padding = { 6.0f, 6.0f };
    const vec2 console_pos = { 32.0f, 32.0f };
    const vec2 console_size = { 480.0f, float(g_font.get_height() + console_text_padding.y) };

    /* Quad shader */
    g_quad_shader.use_program();
    g_quad_shader.upload_mat4("u_proj", mat4::orthographic(0.0f, 0.0f, float(window_w), float(window_h), -1.0f, 1.0f).e);
    g_quad_vao.bind_vao();

    g_batcher.begin();

    /* Blend for quads, disabled before rendering text batch */
    GL_CHECK(glEnable(GL_BLEND));

    /* Draw console input region */ {
        mat4 model_m = mat4::identity();
        model_m *= mat4::scale(console_size.x, console_size.y, 1.0f);
        model_m *= mat4::translate(console_pos.x, console_pos.y, 0.0f);

        g_quad_shader.upload_vec4("u_color", vec4{ 0.0f, 0.0f, 0.0f, 0.65f }.e);
        g_quad_shader.upload_mat4("u_model", model_m.e);

        GL_CHECK(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL));

        g_batcher.push_text_formatted(console_pos + console_text_padding, g_font, g_input_buffer.c_str());
    }

    /* Draw console history region */ {
        const vec2 history_pos = console_pos + vec2{ 0.0f, console_size.y };
        const vec2 history_size = { console_size.x, float(CONSOLE_HISTORY_MAX * g_font.get_height() + console_text_padding.y) };
        const float history_text_x = console_pos.x + console_text_padding.x;
        float history_text_y = history_pos.y + console_text_padding.y;

        mat4 model_m = mat4::identity();
        model_m *= mat4::scale(history_size.x, history_size.y, 1.0f);
        model_m *= mat4::translate(history_pos.x, history_pos.y, 0.0f);

        g_quad_shader.upload_vec4("u_color", vec4{ 0.0f, 0.0f, 0.0f, 0.45f }.e);
        g_quad_shader.upload_mat4("u_model", model_m.e);

        GL_CHECK(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL));

        for(int32_t history_index = 0; history_index < CONSOLE_HISTORY_MAX; ++history_index) {
            g_batcher.push_text({ history_text_x, history_text_y }, g_font, g_history[history_index].c_str());
            history_text_y += g_font.get_height();
        }
    }

    GL_CHECK(glDisable(GL_BLEND));

    /* Draw cursor */ 
    if(!g_cursor_blink) {
        glDisable(GL_DEPTH_TEST);
        const vec2 cursor_size = { 2.0f, console_size.y - console_text_padding.y };

        vec2 cursor_pos = {
            console_pos.x + console_text_padding.x + g_font.calc_text_width(g_input_buffer.c_str()),
            console_pos.y + console_text_padding.y * 0.5f
        };

        mat4 model_m = mat4::identity();
        model_m *= mat4::scale(cursor_size.x, cursor_size.y, 1.0f);
        model_m *= mat4::translate(cursor_pos.x, cursor_pos.y, 0.0f);

        g_quad_shader.upload_vec4("u_color", vec4{ 1.0f, 1.0f, 1.0f, 1.0f }.e);
        g_quad_shader.upload_mat4("u_model", model_m.e);

        GL_CHECK(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL));
        glEnable(GL_DEPTH_TEST);
    }

    g_batcher.render(window_w, window_h, g_font);
}

void Console::set_open_state(bool open) {
    if(g_is_open == open) {
        return;
    }

    if(open) {
        g_input_buffer.clear();
        SDL_StartTextInput();
    } else {
        SDL_StopTextInput();
    }

    g_is_open = open;
}

bool Console::is_open(void) {
    return g_is_open;
}

// @todo Check on command name and stuff?
void Console::register_command(ConsoleCommand command) {
    g_commands.push_back(command);
}
