#pragma once

#include "common.h"
#include "world.h"
#include "chunk.h"
#include "camera.h"
#include "input.h"

class Game {
public:
    CLASS_COPY_DISABLE(Game);

    /* Initializes and deletes game world and stuff */
    Game(void);
    ~Game(void);

    void update(const Input &input, double delta_time);
    void render(const Window &window);

    void hotload_stuff(void);

    World        &get_world(void);
    const World  &get_world(void) const;
    Camera       &get_camera(void);
    const Camera &get_camera(void) const;

private:
    Camera m_camera;
    World  m_world;
    
    /* resources */
    ShaderFile m_block_shader;
    Texture    m_block_atlas;
};

struct WorldPosition {
    static WorldPosition from_real(const vec3 &real);
    vec3  real;
    vec3i block;
    vec3i block_rel;
    vec2i chunk;
};

vec3i block_position_from_real(const vec3 &real);
vec2i chunk_position_from_block(const vec3i &block);
vec3i block_relative_from_block(const vec3i &block);