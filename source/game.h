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

#define MAX_GEN_CHUNKS_THREADS 2
#define MAX_GEN_MESHES_THREADS 8

int32_t get_load_radius(void);
int32_t get_deload_radius(void);

enum class WorldBlitMode {
    COLOR   = 0,
    NORMALS = 1,
    DEPTH   = 2,
    AMBIENT_OCCLUSION = 3
};

struct GameThreadInfo {
    int32_t gen_chunks_threads_active;
    int32_t gen_meshes_threads_active;
    int32_t queued_chunks_num;
    int32_t queued_meshes_num;
};

struct GameTime {
    double delta_time;
    double elapsed_time;
};

class Game {
public:
    CLASS_COPY_DISABLE(Game);

    /* Initializes and deletes game world and stuff */
    Game(void);
    ~Game(void);

    /* Update the game state */
    void update(const Input &input, double delta_time);

    /**/
    void resize(int32_t width, int32_t height);

    /* Hotload shaders if modified */
    void hotload_shaders(void);

    World   &get_world(void);
    Camera  &get_camera(void);
    Console &get_console(void);

    /* Threading */
    /* @TODO : Chunk meshes sometimes do not get updated correctly if there are more queued !!! */
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
    WorldBlitMode debug_world_blit_mode;

    // todo
    void set_load_radius(int32_t load) {
        m_load_radius = load;
    }
    
    int32_t get_load_radius(void) {
        return m_load_radius;
    }


    GameThreadInfo get_thread_info(void) {
        return {
            m_gen_chunks_thread_num,
            m_gen_meshes_thread_num,
            m_world.get_queued_chunks_num(),
            m_world.get_queued_meshes_num()
        };
    }

    GameTime get_time(void) {
        return {
            m_delta_time,
                m_time_elapsed
        };
    }

    double get_elapsed_time(void) {
        return m_time_elapsed;
    }

private:
    void add_console_commands(void);
    void queue_new_chunks_to_load(void);

    /* Game state */
    double  m_time_elapsed;
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

