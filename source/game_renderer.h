#pragma once

#include "common.h"
#include "framebuffer.h"
#include "shader.h"
#include "cubemap.h"
#include "vertex_array.h"
#include "texture_array.h"
#include "font.h"
#include "text_batcher.h"
#include "camera.h"

class Game;

enum class GameRenderMode {
    NORMAL        = 0,
    DEBUG_NORMALS = 1,
    DEBUG_DEPTH   = 2,
    DEBUG_AO      = 3
};

enum class GameSkyboxChoice { 
    CLEAN,
    CLOUDY,
    WEIRD
};

class GameRenderer {
public:
    CLASS_COPY_DISABLE(GameRenderer);
    
    GameRenderer(int32_t width, int32_t height);
    ~GameRenderer(void);

    /* Resize framebuffers */
    void resize(int32_t width, int32_t height);

    /* Hotload shaders if changed */
    void hotload_shaders(void);

    /* Render game frame */
    void render_frame(Game &game); 

private:
    void render_player(Game &game);
    void render_player_debug(Game &game);
    void render_world(Game &game);
    void render_skybox(Game &game);
    void render_held_block(Game &game);
    void render_target_block(Game &game);
    void render_crosshair(Game &game);
    void render_debug_ui(Game &game);

    void render_cube_outline(const vec3d &position, const vec3d &size, float width, const vec3 &color, float border_perc = 0.0f, const vec3 &border_color = { });
    void render_cube_line_outline(const vec3d &position, const vec3d &size, const vec3 &color);
    void render_line(const vec3d &point_a, const vec3d &point_b, float width, const vec3 &color);

    Cubemap &get_skybox_cubemap(GameSkyboxChoice choice);

    /* Render frame state */
    Camera m_camera;
    mat4   m_proj_m;
    mat4   m_view_m;
    double m_frustum[6][4];

    vec3 m_fog_color;

    /* Window state */
    int32_t m_width;
    int32_t m_height;
    double  m_aspect;

    /* FBOs */
    Framebuffer m_fbo_ms;
    Framebuffer m_fbo;

    /* Fonts */
    Font m_ui_font;
    Font m_ui_font_big;
    Font m_ui_font_smooth;

    /* Common text batcher */
    TextBatcher m_batcher;

    /* Chunk's blocks */
    ShaderFile   m_chunk_shader;
    TextureArray m_chunk_tex_array;

    /* Chunk's water */
    ShaderFile   m_water_shader;
    TextureArray m_water_tex_array;

    /* Skybox */
    VertexArray m_skybox_vao;
    ShaderFile  m_skybox_shader;
    Cubemap     m_skybox_clean;
    Cubemap     m_skybox_cloudy;
    Cubemap     m_skybox_weird;

    /* Resolving framebuffer */
    ShaderFile  m_post_process_shader;
    VertexArray m_post_process_vao;

    /* Crosshair */
    ShaderFile  m_crosshair_shader;
    VertexArray m_crosshair_vao;

    /* For single block rendering */
    VertexArray m_block_vao; 

    /* Cube outline */
    ShaderFile  m_cube_outline_shader;
    VertexArray m_cube_outline_vao;

    /* Cube line outline */
    ShaderFile  m_cube_line_outline_shader;
    VertexArray m_cube_line_outline_vao;

    /* Simple line */
    ShaderFile  m_line_shader;
    VertexArray m_line_vao;
};
