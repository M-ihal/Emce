#pragma once

#include "common.h"
#include "world.h"
#include "chunk.h"
#include "camera.h"
#include "input.h"
#include "player.h"

class Window;

class Game {
public:
    CLASS_COPY_DISABLE(Game);

    /* Initializes and deletes game world and stuff */
    Game(void);
    ~Game(void);

    void update(const Input &input, double delta_time);
    void render(const Window &window);

    void hotload_stuff(void);

    World  &get_world(void);
    Camera &get_camera(void);
    Player &get_player(void);

private:
    void push_debug_ui(void);

    Camera m_camera;
    World  m_world;
    Player m_player;
    
    /* resources */
    ShaderFile m_block_shader;
    Texture    m_block_atlas;
};

