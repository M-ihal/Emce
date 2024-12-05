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

class Game {
public:
    CLASS_COPY_DISABLE(Game);

    /* Initializes and deletes game world and stuff */
    Game(void);
    ~Game(void);

    /* Update the game state */
    void update(const Input &input, double delta_time);

    /* Renders frame onto backbuffer */
    void render_frame(void);

    /* Hotload shaders if modified */
    void hotload_shaders(void);

    /* Window size changed callback */
    void resize(int32_t width, int32_t height);

    void check_for_chunks_to_load(void);

    void wake_up_gen_chunks_threads(void);
    void wake_up_gen_meshes_threads(void);

    World   &get_world(void);
    Camera  &get_camera(void);
    Console &get_console(void);

    /* Threads */
    /* @TODO : Chunk meshes sometimes do not get updated correctly if there are more queued !!! */
    int32_t thread_gen_chunks_proc(void);
    int32_t thread_gen_meshes_proc(void);
    void start_threads(int32_t chunks_threads, int32_t meshes_threads);
    void start_threads(void); // Creates threads with specified last amount or 1, 1
    void stop_threads(void);

private:
    void add_console_commands(void);

    void render_world(const mat4 &proj_m);
    void render_water(const mat4 &proj_m);
    void render_skybox(const mat4 &proj_m);
    void render_target_block(const mat4 &proj_m);
    void render_held_block(const mat4 &proj_m, float aspect_ratio);
    void render_crosshair(void);
    void render_ui(void);
    void render_ui_debug_info(void);
    void render_single_block(BlockType type, const mat4 &model_m, const mat4 &proj_m, const mat4 &view_m);

    /* Game state */
    double  m_time_elapsed;
    double  m_delta_time;
    Camera  m_camera;
    World   m_world;
    Console m_console;
 
    /* Threading stuff */
    int32_t m_gen_chunks_thread_num = 0;
    int32_t m_gen_meshes_thread_num = 0;
    SDL_Thread *m_gen_chunks_threads[MAX_GEN_CHUNKS_THREADS] = { };
    SDL_Thread *m_gen_meshes_threads[MAX_GEN_MESHES_THREADS] = { };
    bool m_threads_started = false;
    bool m_threads_keep_looping;
    SDL_Condition *m_gen_chunks_condition;
    SDL_Condition *m_gen_meshes_condition;

    /*
     *  Render resources
     */

    /* Initializes Game's rendering resources */
    void initialize_render_resources(void);

    /* Initializes Game's rendering resources */
    void delete_render_resources(void);

    int32_t m_width;
    int32_t m_height;
    Framebuffer m_fbo_ms;
    Framebuffer m_fbo;

    Font m_ui_font;
    Font m_ui_font_big;
    Font m_ui_font_smooth;

    /* Chunk's blocks */
    ShaderFile   m_chunk_shader;
    TextureArray m_chunk_tex_array;

    /* Chunk's water */
    ShaderFile   m_water_shader;
    TextureArray m_water_tex_array;

    /* Skybox */
    VertexArray m_skybox_vao;
    ShaderFile  m_skybox_shader;
    Cubemap     m_skybox_cubemap;

    /* Resolving framebuffer */
    ShaderFile  m_post_process_shader;
    VertexArray m_post_process_vao;

    /* Crosshair */
    ShaderFile  m_crosshair_shader;
    VertexArray m_crosshair_vao;

    /* For single block rendering */
    VertexArray m_block_vao; 

    TextBatcher m_batcher;
};

enum class WorldBlitMode {
    COLOR,
    NORMALS,
    DEPTH,
    AMBIENT_OCCLUSION
};
