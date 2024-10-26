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

    World   &get_world(void);
    Camera  &get_camera(void);
    Player  &get_player(void);
    Console &get_console(void);

private:
    void push_debug_ui(void);
    void add_console_commands(void);

    double m_time_elapsed;
    double m_delta_time;

    Camera  m_camera;
    World   m_world;
    Player  m_player;
    Console m_console;
 
    Framebuffer m_fbo_world; /* 3d world */
    Framebuffer m_fbo_ui;    /* 2d UI*/

    Font m_ui_font;
    Font m_ui_font_big;

    ShaderFile  m_block_shader;
    Texture     m_block_atlas;

    VertexArray m_skybox_vao;
    ShaderFile  m_skybox_shader;
    Cubemap     m_skybox_cubemap;
};

