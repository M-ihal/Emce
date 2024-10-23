#pragma once

#include "common.h"
#include "world.h"
#include "chunk.h"
#include "camera.h"
#include "input.h"
#include "player.h"
#include "framebuffer.h"
#include "cubemap.h"

class Window;

class Game {
public:
    CLASS_COPY_DISABLE(Game);

    /* Initializes and deletes game world and stuff */
    Game(Window &window);
    ~Game(void);

    void update(Window &window, const Input &input, double delta_time);

    void render_world(void);
    void render_ui(void);

    void hotload_stuff(void);
    void resize_framebuffers(int32_t new_width, int32_t new_height);

    World  &get_world(void);
    Camera &get_camera(void);
    Player &get_player(void);

private:
    void push_debug_ui(void);
    void add_console_commands(void);

    Camera m_camera;
    World  m_world;
    Player m_player;
 
    Framebuffer m_fbo_world; /* 3d world */
    Framebuffer m_fbo_ui;    /* 2d UI*/

    ShaderFile  m_block_shader;
    Texture     m_block_atlas;

    VertexArray m_skybox_vao;
    ShaderFile  m_skybox_shader;
    Cubemap     m_skybox_cubemap;
};

