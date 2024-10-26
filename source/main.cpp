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
#include "debug_ui.h"
#include "game.h"
#include "simple_draw.h"
#include "framebuffer.h"
#include "draw.h"

int32_t calc_average_fps(double delta_time);

namespace {
    constexpr int32_t INIT_WINDOW_WIDTH  = 1280;
    constexpr int32_t INIT_WINDOW_HEIGHT = 720;
    const char       *INIT_WINDOW_TITLE  = "emce";
}

struct MutexTicket {
    int64_t volatile ticket;
    int64_t volatile serving;
};

void begin_ticket_mutex(MutexTicket &mutex) {
    int64_t ticket = _InterlockedExchangeAdd64((__int64 volatile *)&mutex.ticket, 1);
    while(ticket != mutex.serving);
}

void end_ticket_mutex(MutexTicket &mutex) {
    _InterlockedExchangeAdd64((__int64 volatile *)&mutex.serving, 1);
}

MutexTicket mutex_load_queue = { };
MutexTicket mutex_gen_queue  = { };
MutexTicket mutex_get_chunk  = { };

int chunk_gen_thread_func(void *param) {
    Game *game = (Game *)param;

    while(true) {
        game->get_world().process_load_queue();
    }

    return 0;
}

int SDL_main(int argc, char *argv[]) {
    /* init std randomizer */
    srand(time(NULL));

    const bool sdl_success = SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO);
    if(!sdl_success) {
        fprintf(stderr, "[error] SDL: Failed to initialize SDL, Error: %s.\n", SDL_GetError());
        return -1;
    }

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

    /* Initialize global stuff */
    TextBatcher::initialize();
    DebugUI::initialize();
    SimpleDraw::initialize();


    Input input;
    Game  game(window);

    game.get_console().set_command("quit", { CONSOLE_COMMAND_LAMBDA {
            window.set_should_close();
        }
    });

    const double time_freq = SDL_GetPerformanceFrequency();
    uint64_t time_now  = SDL_GetPerformanceCounter();
    uint64_t time_last = SDL_GetPerformanceCounter();
    double elapsed_time = 0.0;
    double delta_time   = 0.0;

    /* Create threads */
    const uint32_t chunk_thread_count = 1;
    SDL_Thread *threads[chunk_thread_count] = { };
    for(uint32_t index = 0; index < chunk_thread_count; ++index) {
        char thread_name[64];
        sprintf_s(thread_name, ARRAY_COUNT(thread_name), "Chunk gen thread #%u", index);
        threads[index] = SDL_CreateThread(chunk_gen_thread_func, thread_name, (void *)&game);
        SDL_DetachThread(threads[index]);
    }

    while(window.get_should_close()) {
        window.process_events(input);

        if(window.size_changed_this_frame()) {
            game.resize_framebuffers(window.get_width(), window.get_height());
        }

        DebugUI::begin_frame(window.get_width(), window.get_height());

        /* Calculate time */ {
            time_now = SDL_GetPerformanceCounter();
            const double delta = time_now - time_last;
            delta_time = delta / time_freq;
            clamp_max_v(delta_time, 1.0 / 30.0);

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

        /* Update */ {
            if(input.key_pressed(Key::F03)) {
                DebugUI::toggle();
            }

            game.update(window, input, delta_time);
        }

        /* Render */ {
            SimpleDraw::set_camera(game.get_camera(), window.calc_aspect());

            glViewport(0, 0, window.get_width(), window.get_height());
            GL_CHECK(glClearColor(0.2f, 0.2f, 0.4f, 1.0));
            GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

            game.render_world();
            game.render_ui();
        }

        window.swap_buffer();
    }

    /* Delete global stuff */
    SimpleDraw::destroy();
    DebugUI::destroy();
    TextBatcher::destroy();

    return 0;
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
