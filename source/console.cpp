#include "console.h"
#include "window.h"
#include "input.h"
#include "opengl_abs.h"

#include <cstdarg>
                 
#define STRING_VIU_CPP_HELPERS
#include <string_viu.h>

#include <meh_hash.h>

void Console::initialize(void) {
    ASSERT(!m_initialized);
    m_initialized = true;

    m_commands.initialize_table(64);

    /* Load font */
    m_font.load_from_file("C://dev//emce//data//MinecraftRegular-Bmg3.otf", 20);
    m_font.get_atlas().set_filter_min(TextureFilter::NEAREST);
    m_font.get_atlas().set_filter_mag(TextureFilter::NEAREST);

    m_quad_shader.set_filepath_and_load("C://dev//emce//source//shaders//simple_quad.glsl");

    /* Initialize quad vertex array */ {
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

        m_quad_vao.create_vao(layout, ArrayBufferUsage::STATIC);
        m_quad_vao.set_ibo_data(ibo_data, ARRAY_COUNT(ibo_data));
        m_quad_vao.set_vbo_data(vbo_data, ARRAY_COUNT(vbo_data) * sizeof(float));
        m_quad_vao.apply_vao_attributes();
    }

    this->set_command("clear", CONSOLE_COMMAND_LAMBDA { console.clear_history(); });
}

void Console::destroy(void) {
    m_font.delete_font();
    m_quad_shader.delete_shader_file();
    m_quad_vao.delete_vao();
    m_commands.delete_table();
    m_initialized = false;
}

static void parse_command(const char *buffer, StringViu &out_command, std::vector<StringViu> &out_args) {
    StringViu command_view = s_viu(buffer);
    command_view = s_viu_trim(command_view);

    out_command = { };
    out_args.clear();

    StringViu command = { };
    StringViu arg1    = { };
    StringViu arg2    = { };
    StringViu arg3    = { };

    int32_t space_idx = s_viu_find_first(command_view, ' ');
    if(space_idx == -1) {
        command = command_view;
        goto command_parsed;
    }

    StringViu args;
    s_viu_split(command_view, space_idx, command, args);
    args = s_viu_trim(args);

    space_idx = s_viu_find_first(args, ' ');
    if(space_idx == -1) {
        arg1 = args;
        goto command_parsed;
    }

    s_viu_split(args, space_idx, arg1, args);
    args = s_viu_trim(args);

    space_idx = s_viu_find_first(args, ' ');
    if(space_idx == -1) {
        arg2 = args;
        goto command_parsed;
    }

    s_viu_split(args, space_idx, arg2, args);
    args = s_viu_trim(args);

    space_idx = s_viu_find_first(args, ' ');
    if(space_idx == -1) {
        arg3 = args;
        goto command_parsed;
    }

    s_viu_split(args, space_idx, arg3, args);

command_parsed:

    out_command = command;
    if(arg1.length) out_args.push_back(arg1);
    if(arg2.length) out_args.push_back(arg2);
    if(arg3.length) out_args.push_back(arg3);
}

void Console::update(Game &game, const Input &input, Window &window, double delta_time) {
    if(!m_is_open) {
        return;
    }

    bool input_changed_this_frame = false;


    /* Get last valid input */
    if(input.key_pressed(Key::UP)) {
        m_input = m_last_entered;
        input_changed_this_frame = true;
    }

    /* Delete text */
    if(input.key_pressed_or_repeat(Key::BACKSPACE) && m_input.length()) {
        m_input.pop_back();
        input_changed_this_frame = true;
    }

    /* Query text input */
    const char *text_input = input.get_text_input();
    size_t input_len = strlen(text_input);
    if(input_len) {
        for(int32_t index = 0; index < input_len && m_input.length() < CONSOLE_INPUT_MAX; ++index) {
            m_input += text_input[index];
            input_changed_this_frame = true;
        }
    }

    if(input.key_pressed(Key::ENTER) && m_input.length()) {
        /* Parse the input buffer */
        StringViu command;
        std::vector<StringViu> args;
        parse_command(m_input.c_str(), command, args);

        CommandTableKey command_key;
        sprintf_s(command_key.string, ARRAY_COUNT(command_key.string), "%.*s", command.length, command.data);

        console_command_proc **command_proc = m_commands.find(command_key);
        if(command_proc != NULL) {
            (*command_proc)(*this, args, window, game);
        } else {
            /* Custom command */
            if(command == "commands") {
                this->add_to_history("> Commands:");
                ConsoleCommandTable::Iterator iter;
                while(m_commands.iterate_all(iter)) {
                    this->add_to_history(iter.key.string);
                }
            }
        }

        m_last_entered = m_input;
        m_input = "";
    }

    /* Blink cursor */
    if(input_changed_this_frame) {
        m_cursor_blink_t = 0.0;
        m_cursor_blink = false;
    } else {
        m_cursor_blink_t += delta_time;
        if(m_cursor_blink_t >= CONSOLE_CURSOR_BLINK_TIME) {
            m_cursor_blink_t = 0.0;
            BOOL_TOGGLE(m_cursor_blink);
        }
    }
}

void Console::render(int32_t width, int32_t height) {
    if(!m_is_open) {
        return;
    }

    const vec2 console_text_padding = { 6.0f, 6.0f };
    const vec2 console_pos  = { 32.0f, 32.0f };
    const vec2 console_size = { 480.0f, float(m_font.get_height() + console_text_padding.y) };

    /* Quad shader */
    m_quad_shader.use_program();
    m_quad_shader.upload_mat4("u_proj", mat4::orthographic(0.0f, 0.0f, float(width), float(height), -1.0f, 1.0f));
    m_quad_vao.bind_vao();

    m_batcher.begin();

    set_render_state({
        .blend = BlendFunc::SRC_ALPHA__ONE_MINUS_SRC_ALPHA,
        .depth = DepthFunc::DISABLE,
        .multisample = true
    });

    /* Draw console input region */ {
        mat4 model_m = mat4::identity();
        model_m *= mat4::scale(console_size.x, console_size.y, 1.0f);
        model_m *= mat4::translate(console_pos.x, console_pos.y, 0.0f);

        m_quad_shader.upload_vec4("u_color", vec4{ 0.0f, 0.0f, 0.0f, 0.85f }.e);
        m_quad_shader.upload_mat4("u_model", model_m);

        draw_elements_triangles(6);

        m_batcher.push_text_formatted(console_pos + console_text_padding, m_font, m_input.c_str());
    }

    /* Draw console history region */ {
        const vec2  history_pos = console_pos + vec2{ 0.0f, console_size.y };
        const vec2  history_size = { console_size.x, float(CONSOLE_HISTORY_MAX * m_font.get_height() + console_text_padding.y) };
        const float history_text_x = console_pos.x + console_text_padding.x;
        float       history_text_y = history_pos.y + console_text_padding.y;

        mat4 model_m = mat4::identity();
        model_m *= mat4::scale(history_size.x, history_size.y, 1.0f);
        model_m *= mat4::translate(history_pos.x, history_pos.y, 0.0f);

        m_quad_shader.upload_vec4("u_color", vec4{ 0.0f, 0.0f, 0.0f, 0.65f }.e);
        m_quad_shader.upload_mat4("u_model", model_m);

        draw_elements_triangles(6);

        for(int32_t history_index = 0; history_index < CONSOLE_HISTORY_MAX; ++history_index) {
            m_batcher.push_text({ history_text_x, history_text_y }, m_font, m_history[history_index].c_str());
            history_text_y += m_font.get_height();
        }
    }

    /* Draw cursor */ 
    if(!m_cursor_blink) {
        const vec2 cursor_size = { 2.0f, console_size.y - console_text_padding.y };

        vec2 cursor_pos = {
            console_pos.x + console_text_padding.x + m_font.calc_text_width(m_input.c_str()),
            console_pos.y + console_text_padding.y * 0.5f
        };

        mat4 model_m = mat4::identity();
        model_m *= mat4::scale(cursor_size.x, cursor_size.y, 1.0f);
        model_m *= mat4::translate(cursor_pos.x, cursor_pos.y, 0.0f);

        m_quad_shader.upload_vec4("u_color", vec4{ 1.0f, 1.0f, 1.0f, 1.0f }.e);
        m_quad_shader.upload_mat4("u_model", model_m);

        draw_elements_triangles(6);
    }

    m_batcher.render(width, height, m_font, { 1.0f, 1.0f, 1.0f });
}

void Console::add_to_history(const char *format, ...) {
    /* Push history */
    for(uint32_t index = CONSOLE_HISTORY_MAX - 1; index >= 1; --index) {
        m_history[index] = m_history[index - 1];
    }

    static char buffer[CONSOLE_INPUT_MAX + 1];
    ZERO_ARRAY(buffer);

    va_list args;
    va_start(args, format);
    vsprintf_s(buffer, ARRAY_COUNT(buffer), format, args);
    va_end(args);

    m_history[0] = std::string(buffer);
}

void Console::clear_history(void) {
    for(uint32_t index = 0; index < CONSOLE_HISTORY_MAX; ++index) {
        m_history[index] = "";
    }
}

void Console::set_open_state(bool open, Window &window) {
    if(m_is_open == open) {
        return;
    }
 
    if(open) { m_input = ""; }

    m_is_open = open;
    window.set_text_input_active(open);
}

bool Console::is_open(void) {
    return m_is_open;
}

void Console::set_command(const char command_name[CONSOLE_COMMAND_NAME_MAX], console_command_proc *command_proc) {
    CommandTableKey key = { };

    // @todo
    if(strlen(command_name) >= (CONSOLE_COMMAND_NAME_MAX - 1)) {
        fprintf(stderr, "Error: Console: set_command(...), too big name for command.");
        return;
    }

    sprintf_s(key.string, CONSOLE_COMMAND_NAME_MAX, command_name);
    m_commands.insert(key, command_proc);
}
