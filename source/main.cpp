#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_thread.h>
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
#include "game.h"
#include "game_renderer.h"
#include "framebuffer.h"
#include "opengl_abs.h"

#include <filesystem>

#define STRING_VIU_CPP_HELPERS
#include <string_viu.h>

int SDL_main(int argc, char *argv[]) {

    GameInitConfig game_init_config;
    for(int32_t arg_index = 0; arg_index < argc; ++arg_index) {
        StringViu arg_view = s_viu(argv[arg_index]);
        StringViu arg_rem;
        if(s_viu_begins_with(arg_view, &arg_rem, "-th_chunks=")) {
            if(arg_rem.length > 0) {
                std::string value_str = s_viu_to_std_string(arg_rem);
                try {
                    int32_t value = std::stoi(value_str);
                    game_init_config.init_chunks_threads = value;
                } catch(...) {
                    /* Failed to parse command line argument value... Do not set anything */
                }
            }
        } else if(s_viu_begins_with(arg_view, &arg_rem, "-th_meshes=")) {
            if(arg_rem.length > 0) {
                std::string value_str = s_viu_to_std_string(arg_rem);
                try {
                    int32_t value = std::stoi(value_str);
                    game_init_config.init_meshes_threads = value;
                } catch(...) {
                    /* Failed to parse command line argument value... Do not set anything */
                }
            }
        } else if(s_viu_begins_with(arg_view, &arg_rem, "-load_radius=")) {
            if(arg_rem.length > 0) {
                std::string value_str = s_viu_to_std_string(arg_rem);
                try {
                    int32_t value = std::stoi(value_str);
                    game_init_config.load_radius = value;
                } catch(...) {
                    /* Failed to parse command line argument value... Do not set anything */
                }
            }
        }
    }

    /* Set working directory to folder above .exe file @TODO */ {
        std::filesystem::path folder = std::filesystem::path(SDL_GetBasePath()).parent_path().parent_path();
        fprintf(stdout, "[info] Setting working directory to: \"%s\"\n", folder.string().c_str());
        std::filesystem::current_path(folder);
    }

    /* init std randomizer */
    srand(time(NULL));

    const bool sdl_success = SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO);
    if(!sdl_success) {
        fprintf(stderr, "[error] SDL: Failed to initialize SDL, Error: %s.\n", SDL_GetError());
        return -1;
    }

    /* Initialize window */
    try {
        Window::get();
    } catch(const std::exception &exc) {
        fprintf(stderr, "[error] Window: Failed to create window.\n");
        return -1;
    }

    Window &window = Window::get();

    const bool glew_success = glewInit() == GLEW_OK;
    if(!glew_success) {
        fprintf(stderr, "[error] Glew: Failed to initialize.\n");
        return -1;
    }

    /* Initialize global stuff */
    TextBatcher::initialize();

    Input input;
    GameRenderer renderer(window.get_width(), window.get_height());
    std::unique_ptr<Game> game = std::unique_ptr<Game>(new Game(game_init_config));

    game->get_console().set_command("quit", { CONSOLE_COMMAND_LAMBDA {
            Window::get().set_should_close();
        }
    });

    const double time_freq = SDL_GetPerformanceFrequency();
    uint64_t time_now  = SDL_GetPerformanceCounter();
    uint64_t time_last = SDL_GetPerformanceCounter();
    double elapsed_time = 0.0;
    double delta_time   = 0.0;

    while(window.get_should_close()) {
        window.process_events(input);

        if(window.size_changed_this_frame()) {
            renderer.resize(window.get_width(), window.get_height());
        }

        /* Calculate time */ {
            time_now = SDL_GetPerformanceCounter();
            const double delta = time_now - time_last;
            delta_time = delta / time_freq;
            clamp_max_v(delta_time, 1.0 / 30.0);

            elapsed_time += delta_time;
            time_last = time_now;
        }

        /* Hotload stuff here */
        TextBatcher::hotload_shader();
        renderer.hotload_shaders();

        game->update(input, delta_time);

        /* Clear backbuffer */
        bind_no_fbo();
        set_viewport({ 0, 0, window.get_width(), window.get_height() });
        GL_CHECK(glClearColor(0.2f, 0.2f, 0.4f, 1.0));
        GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));

        renderer.render_frame(*game);

        window.swap_buffer();
    }

    /* Delete global stuff */
    TextBatcher::destroy();

    fprintf(stdout, "[Info] Exited without a crash...\n");
    return 0;
}

