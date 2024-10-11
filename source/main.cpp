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
#include "cubemap.h"
#include "font.h"
#include "text_batcher.h"
#include "debug_ui.h"
#include "console.h"
#include "game.h"
#include "simple_draw.h"

namespace {
    constexpr int32_t INIT_WINDOW_WIDTH  = 1280;
    constexpr int32_t INIT_WINDOW_HEIGHT = 720;
    const char       *INIT_WINDOW_TITLE  = "emce";
}

/*
    @todo: Better cleanup of reasorces... maybe make it explicit not in destructors
    @todo: Maybe make window globally accesible

    @todo @maybe Make every constructor end destructor explicit?
    @todo Make window global?
*/

int32_t calc_average_fps(double delta_time);
void render_random_thing_at_origin(const Camera &camera, float aspect);

#include <windows.h>

DWORD WINAPI chunk_gen_thread(void *param) {
    Game *game = (Game *)param;
    Sleep(100);
    for(;;) {
        Sleep(5);
        game->get_world().process_load_queue();
    }
    return 0;
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
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
    }

    /* Initialize global stuff */
    TextBatcher::initialize();
    DebugUI::initialize();
    SimpleDraw::initialize();
    Console::initialize();

    Console::register_command({
        .command = "quit", 
        .proc = CONSOLE_COMMAND_LAMBDA {
            window.set_should_close();
        }
    });

    Input input;
    Game  game;

    bool show_debug_ui = true;

    const std::string folder = "skybox_blue";
    const std::string extension = ".jpg";
    const std::string filepaths[6] = {
        "C://dev//emce//data//" + folder + "//right" + extension,
        "C://dev//emce//data//" + folder + "//left" + extension,
        "C://dev//emce//data//" + folder + "//up" + extension,
        "C://dev//emce//data//" + folder + "//down" + extension,
        "C://dev//emce//data//" + folder + "//front" + extension,
        "C://dev//emce//data//" + folder + "//back" + extension,
    };

    Cubemap skybox;
    skybox.load_from_file(filepaths);
    skybox.set_filter_min(TextureFilter::NEAREST);
    skybox.set_filter_mag(TextureFilter::NEAREST);

    ShaderFile skybox_shader;
    skybox_shader.set_filepath_and_load("C://dev//emce//source//shaders//skybox.glsl");

    const float sv_vertices[] = {
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f 
    };

    const uint32_t sv_indices[] = {
        0, 1, 2,
        2, 3, 0,
        4, 5, 6,
        6, 7, 4,
        4, 0, 3,
        3, 7, 4,
        1, 5, 6,
        6, 2, 1,
        3, 2, 6,
        6, 7, 3,
        0, 1, 5,
        5, 4, 0
    };

    BufferLayout sv_layout;
    sv_layout.push_attribute("a_position", 3, BufferDataType::FLOAT);

    VertexArray skybox_vao;
    skybox_vao.create_vao(sv_layout, ArrayBufferUsage::STATIC);
    skybox_vao.apply_vao_attributes();
    skybox_vao.set_vbo_data(sv_vertices, ARRAY_COUNT(sv_vertices) * sizeof(float));
    skybox_vao.set_ibo_data(sv_indices, ARRAY_COUNT(sv_indices));

    const double time_freq = SDL_GetPerformanceFrequency();
    uint64_t time_now  = SDL_GetPerformanceCounter();
    uint64_t time_last = SDL_GetPerformanceCounter();
    double elapsed_time = 0.0;
    double delta_time   = 0.0;

#if 0
    DWORD thread_id;
    HANDLE thread = CreateThread(0, 0, chunk_gen_thread, &game, 0, &thread_id);
    CloseHandle(thread);
#endif

    while(window.get_should_close()) {
        window.process_events(input);

        DebugUI::begin_frame(window.get_width(), window.get_height());

        /* Calculate time */ {
            time_now = SDL_GetPerformanceCounter();
            const double delta = time_now - time_last;
            delta_time = delta / time_freq;
#if 0
            if(input.key_is_down(Key::LEFT_CTRL)) {
                delta_time *= 0.1;
            }
#endif
            elapsed_time += delta_time;
            time_last = time_now;
        }

        DebugUI::push_text_left(" --- Frame ---");
        DebugUI::push_text_left("elapsed time: %.3f", elapsed_time);
        DebugUI::push_text_left("frame time: %.6f", delta_time);
        DebugUI::push_text_left("fps: %d", calc_average_fps(delta_time));

        /* Hotload stuff here */
        TextBatcher::hotload_shader();
        SimpleDraw::hotload_shader();
        game.hotload_stuff();
        skybox_shader.hotload();


        /*        */
        /* Update */
        /*        */

        if(input.key_pressed(Key::F03)) {
            show_debug_ui = !show_debug_ui;
        }

        if(input.key_pressed(Key::T) && !Console::is_open()) {
            Console::set_open_state(true);
        }

        if(input.key_pressed(Key::ESCAPE) && !Console::is_open()) {
            window.set_should_close();
        }

        Console::update(input, window, game, delta_time);
        game.update(input, delta_time);

        SimpleDraw::set_camera(game.get_camera(), window.calc_aspect());

        /*        */
        /* Render */
        /*        */

        glViewport(0, 0, window.get_width(), window.get_height());
        GL_CHECK(glClearColor(0.2f, 0.2f, 0.4f, 1.0));
        GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

        game.render(window);
        render_random_thing_at_origin(game.get_camera(), window.calc_aspect());

        /* Render skybox */ {
            glDepthFunc(GL_LEQUAL);
            skybox_shader.use_program();
            skybox_shader.upload_mat4("u_proj", game.get_camera().calc_proj(window.calc_aspect()).e);
            skybox_shader.upload_mat4("u_view", game.get_camera().calc_view().e);
            skybox_vao.bind_vao();
            skybox.bind_cubemap();
            GL_CHECK(glDrawElements(GL_TRIANGLES, skybox_vao.get_ibo_count(), GL_UNSIGNED_INT, NULL));
            glDepthFunc(GL_LESS);
        }

        Console::render(window.get_width(), window.get_height());

        if(show_debug_ui) {
            DebugUI::render();
        }

        window.swap_buffer();
    }

    /* Delete global stuff */
    Console::destroy();
    SimpleDraw::destroy();
    DebugUI::destroy();
    TextBatcher::destroy();

    skybox.delete_cubemap();

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
