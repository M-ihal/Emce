#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <glew.h>

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "common.h"
#include "window.h"
#include "input.h"
#include "shader.h"
#include "vertex_array.h"
#include "math_types.h"
#include "texture.h"
#include "font.h"
#include "text_batcher.h"
#include "debug_ui.h"
#include "console.h"
#include "game.h"

#define INIT_WINDOW_WIDTH  1280
#define INIT_WINDOW_HEIGHT 720
#define INIT_WINDOW_TITLE  "emce"

/*
    @todo: Game / GameManager / Game-Something class
    @todo: Better cleanup of reasorces... maybe make it explicit not in destructors
    @todo: Maybe make window globally accesible

    @todo @maybe Make every constructor end destructor explicit?
    @todo Make window global?
*/

int32_t calc_average_fps(double delta_time);
void render_random_thing_at_origin(const Camera &camera, float aspect);

CONSOLE_COMMAND_PROC(setpos) {
    if(args.size() != 3) {
        Console::add_to_history("> missing arguments");
    } else {
        float x = std::stof(args[0]);
        float y = std::stof(args[1]);
        float z = std::stof(args[2]);
        camera.set_position({ x, y, z });
        char buffer[128];
        sprintf_s(buffer, ARRAY_COUNT(buffer), "> position set to (%.1f, %.1f, %.1f)", x, y, z);
        Console::add_to_history(buffer);
    }
}

void register_commands(void) {
    Console::register_command({
        .command = "quit", 
        .proc = CONSOLE_COMMAND_LAMBDA {
            window.should_quit();
        }
    });

    Console::register_command({
        .command = "cam", 
        .proc = CONSOLE_COMMAND_LAMBDA {
            camera.set_position({ -54.0f, 128.0f, 37.0f });
            camera.set_rotation({ DEG_TO_RAD(90.0f), DEG_TO_RAD(-45.0f) });
        }
    });

    Console::register_command({
        .command = "dog", 
        .proc = CONSOLE_COMMAND_LAMBDA {
            camera.set_position({ 0.63f, 129.88f, 5.57f });
            camera.set_rotation({ DEG_TO_RAD(266.510f), DEG_TO_RAD(3.128f) });
        }
    });

    Console::register_command({
        .command = "setpos", 
        .proc = setpos
    });
}

int SDL_main(int argc, char *argv[]) {
    /* init std randomizer */
    srand(time(NULL));

    const bool sdl_success = SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO) == 0;
    if(!sdl_success) {
        fprintf(stderr, "[error] SDL: Failed to initialize SDL, Error: %s.\n", SDL_GetError());
        return -1;
    }

    /* Do not query text input by default */
    SDL_StopTextInput();

    Window window;

    if(!window.initialize(INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT, INIT_WINDOW_TITLE)) {
        fprintf(stderr, "[error] Window: Failed to create valid opengl window.\n");
        return -1;
    }

    const bool glew_success = glewInit() == GLEW_OK;
    if(!glew_success) {
        fprintf(stderr, "[error] Glew: Failed to initialize.\n");
        return -1;
    }

    /* Initialize OpenGL state */ {
        GL_CHECK(glEnable(GL_DEPTH_TEST));
        GL_CHECK(glDepthFunc(GL_LESS));
    }

    /* Initialize global stuff */
    TextBatcher::initialize();
    DebugUI::initialize();
    Console::initialize();
    register_commands();

    Input input;
    Game  game;

    const double time_freq = SDL_GetPerformanceFrequency();
    uint64_t time_now  = SDL_GetPerformanceCounter();
    uint64_t time_last = SDL_GetPerformanceCounter();
    double elapsed_time = 0.0;
    double delta_time   = 0.0;

    while(window.is_running()) {
        window.process_events(input);

        DebugUI::begin_frame(window.width(), window.height());

        /* Calculate time */ {
            time_now = SDL_GetPerformanceCounter();
            const double delta = time_now - time_last;
            delta_time = delta / time_freq;
            elapsed_time += delta_time;
            time_last = time_now;
        }
        
        DebugUI::push_text_left(" --- Frame ---");
        DebugUI::push_text_left("elapsed time: %.3f", elapsed_time);
        DebugUI::push_text_left("frame time: %.6f", delta_time);
        DebugUI::push_text_left("fps: %d", calc_average_fps(delta_time));

        /* Hotload stuff here */
        TextBatcher::hotload_shader();
        game.hotload_stuff();


        /*        */
        /* Update */
        /*        */

        if(input.key_pressed(Key::T) && !Console::is_open()) {
            Console::set_open_state(true);
        }

        if(input.key_pressed(Key::ESCAPE) && !Console::is_open()) {
            window.should_quit();
        }

        Console::update(input, window, game.get_camera());
        game.update(input, delta_time);


        /*        */
        /* Render */
        /*        */

        glViewport(0, 0, window.width(), window.height());
        GL_CHECK(glClearColor(0.2f, 0.2f, 0.4f, 1.0));
        GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

        game.render(window);
        render_random_thing_at_origin(game.get_camera(), window.calc_aspect());
        Console::render(window.width(), window.height());
        DebugUI::render();


        window.swap_buffers();
    }

    /* Delete global stuff */
    Console::destroy();
    DebugUI::destroy();
    TextBatcher::destroy();

    return 0;
}

void render_random_thing_at_origin(const Camera &camera, float aspect) {
    /// @todo NOT DELETING THOSE....
    static Shader s_shader;
    static VertexArray s_vao;
    static Texture s_texture;
    static bool    s_initialized = false;
    if(!s_initialized) {
        s_initialized = true;
        s_shader.load_from_file("C://dev//emce//source//shaders//flat_image.glsl");
        s_texture.load_from_file("C://dev//emce//data//dog.png", true);

        const float fval = 4.0f;

        const float vertices[] = {
            -fval, -fval, 0.0f, 0.0f, 0.0f,
            +fval, -fval, 0.0f, 1.0f, 0.0f,
            +fval, +fval, 0.0f, 1.0f, 1.0f,
            -fval, +fval, 0.0f, 0.0f, 1.0f,
        };

        const uint32_t indices[] = {
            0, 1, 2,
            2, 3, 0
        };

        BufferLayout layout;
        layout.push_attribute("a_position", 3, BufferDataType::FLOAT);
        layout.push_attribute("a_tex_coord", 2, BufferDataType::FLOAT);

        s_vao.create_vao(layout, ArrayBufferUsage::STATIC);
        s_vao.set_vbo_data(vertices, ARRAY_COUNT(vertices) * sizeof(float));
        s_vao.set_ibo_data(indices, 6);
        s_vao.apply_vao_attributes();
    }

    mat4 proj_m = camera.calc_proj(aspect);
    mat4 view_m = camera.calc_view();
    s_shader.use_program();
    s_shader.upload_mat4("u_proj", proj_m.e);
    s_shader.upload_mat4("u_view", view_m.e);
    s_shader.upload_mat4("u_model", mat4::translate({ 0.0f, 130.0f, 0.0f }).e);
    s_shader.upload_int("u_image", 0);
    s_texture.bind_texture_unit(0);

    s_vao.bind_vao();
    glDrawElements(GL_TRIANGLES, s_vao.get_ibo_count(), GL_UNSIGNED_INT, NULL);
}

int32_t calc_average_fps(double delta_time) {
    static int32_t draw_fps = 0;
    static int32_t frame_times_counter = 0;
    static double frame_times[32] = { };
    frame_times_counter++;
    frame_times_counter %= ARRAY_COUNT(frame_times);
    frame_times[frame_times_counter] = delta_time;
    double average = 0.0f;
    for(int32_t i = 0; i < ARRAY_COUNT(frame_times); ++i) {
        average += frame_times[i];
    }
    const int32_t num_frames = ARRAY_COUNT(frame_times);
    average /= double(num_frames);
    int32_t fps = int32_t(1.0 / average);
    if(frame_times_counter == ARRAY_COUNT(frame_times) - 1) {
        draw_fps = fps;
    }
    return draw_fps;
}
