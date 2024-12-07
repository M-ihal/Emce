#pragma once

#include "common.h"
#include "framebuffer.h"
#include "shader.h"
#include "cubemap.h"
#include "vertex_array.h"
#include "texture_array.h"
#include "font.h"
#include "text_batcher.h"

class Game;

class GameRenderer {
public:
    CLASS_COPY_DISABLE(GameRenderer);
    
    GameRenderer(int32_t width, int32_t height);

    ~GameRenderer(void);

    void resize(int32_t width, int32_t height);

    void hotload_shaders(void);

    void render_frame(Game &game); 

private:
    void render_world(Game &game, const mat4 &view_m, const mat4 &proj_m);
    void render_skybox(Game &game, const mat4 &view_m, const mat4 &proj_m);
    void render_held_block(Game &game, const mat4 &view_m, const mat4 &proj_m);
    void render_target_block(Game &game, const mat4 &view_m, const mat4 &proj_m);
    void render_crosshair(Game &game);
    void render_debug_ui(Game &game);

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
    Cubemap     m_skybox_cubemap;

    /* Resolving framebuffer */
    ShaderFile  m_post_process_shader;
    VertexArray m_post_process_vao;

    /* Crosshair */
    ShaderFile  m_crosshair_shader;
    VertexArray m_crosshair_vao;

    /* For single block rendering */
    VertexArray m_block_vao; 
};
