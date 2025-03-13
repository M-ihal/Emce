#pragma once

#include "common.h"
#include "world.h"
#include "chunk.h"
#include "camera.h"
#include "input.h"
#include "player.h"
#include "framebuffer.h"
#include "cubemap.h"
#include "console.h"
#include "texture_array.h"
#include "game_renderer.h"

#define INIT_GEN_CHUNKS_THREADS 0
#define INIT_GEN_MESHES_THREADS 0
#define INIT_LOAD_RADIUS 24

#define MAX_GEN_CHUNKS_THREADS 2
#define MAX_GEN_MESHES_THREADS 16

/* Optional initial game config values */
struct GameInitConfig {
    int32_t load_radius = INIT_LOAD_RADIUS;
    int32_t init_chunks_threads = INIT_GEN_CHUNKS_THREADS;
    int32_t init_meshes_threads = INIT_GEN_MESHES_THREADS;
};

struct GameFrameInfo {
    double  delta_time;
    double  elapsed_time;
    int32_t load_radius;
    int32_t chunks_threads_active;
    int32_t meshes_threads_active;
    bool    is_3rd_person;
};

class Game {
public:
    CLASS_COPY_DISABLE(Game);

    /* Initializes and deletes game world and stuff */
    Game(GameInitConfig init_flags);
    ~Game(void);
    
    /* Update the game state */
    void update(const Input &input, double delta_time);

    /* Sets chunk load distance */
    void set_load_radius(int32_t radius);

    /* Return bunch of random game variables */
    GameFrameInfo get_frame_info(void);

    /* */
    World   &get_world(void);
    Camera  &get_camera(void);
    Console &get_console(void);

    /* Threading stuff */
    void wake_up_gen_chunks_threads(void);
    void wake_up_gen_meshes_threads(void);
    int32_t thread_gen_chunks_proc(void);
    int32_t thread_gen_meshes_proc(void);
    void start_threads(int32_t chunks_threads, int32_t meshes_threads);
    void start_threads(void); // Creates threads with specified last amount or 1, 1
    void stop_threads(void);

    /* Debug settings */
    bool debug_show_ui_info;
    bool debug_show_chunk_borders;
    bool debug_player_draw;
    bool debug_chunk_wireframe_mode;
    bool debug_raycast_draw;
    vec3i debug_spawn_p1;
    vec3i debug_spawn_p2;

    /* If should generate stuff on main thread if no other threads exist for the task */
    bool gen_on_main_thread = true; 

    /* What mode to render game */
    GameRenderMode render_mode;

    /* Which skybox */
    GameSkyboxChoice skybox_choice;

    bool fog_enable = true;

private:
    /*  */
    void queue_new_chunks_to_load(void);

    /* Update chunks and queue chunks for meshing */
    void update_loaded_chunks(void);

    /* Game state */
    double  m_elapsed_time;
    double  m_delta_time;
    World   m_world;
    Console m_console;
    Camera  m_camera;

    /* Settings */
    int32_t m_load_radius;
    bool    m_3rd_person_mode;
    double  m_3rd_person_distance;

    /* Threading stuff */
    int32_t m_gen_chunks_thread_num = 0;
    int32_t m_gen_meshes_thread_num = 0;
    SDL_Thread *m_gen_chunks_threads[MAX_GEN_CHUNKS_THREADS] = { };
    SDL_Thread *m_gen_meshes_threads[MAX_GEN_MESHES_THREADS] = { };
    bool m_threads_started = false;
    bool m_threads_keep_looping;
    SDL_Condition *m_gen_chunks_condition;
    SDL_Condition *m_gen_meshes_condition;
};

