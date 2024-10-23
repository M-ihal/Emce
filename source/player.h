#pragma once

#include "common.h"
#include "math_types.h"
#include "camera.h"
#include "world.h"

class Input;
class Game;

class Player {
public:
    CLASS_COPY_DISABLE(Player);

    Player(void);
    ~Player(void);

    void update(Game &game, const Input &input, float delta_time);
    void debug_render(Game &game);

    vec3 get_size(void) const;
    void set_position(const vec3 &position);
    vec3 get_position(void) const;
    vec3 get_position_center(void) const;
    vec3 get_position_head(void) const;
    Camera &get_head_camera(void);

    bool is_grounded(void) const;
    vec3 get_velocity(void) const;
    void get_ground_collider_info(vec3 &pos, vec3 &size);

    RaycastBlockResult get_targeted_block(void);

private:
    bool check_if_collides_with_any_block(World &world, vec3 position, vec3 size);
    void move_in_xz(World &world, float delta_time);
    void move_in_y(World &world, float delta_time);
    
    bool   m_is_grounded;
    vec3   m_velocity;
    vec3   m_position;
    bool   m_is_sprinting;
    Camera m_camera;

    /* Targeted block by player */
    RaycastBlockResult m_targeted_block;

    /* Range of blocks that were checked for collision and stuff this frame, for debug display */
    vec3i m_debug_min_checked_block;
    vec3i m_debug_max_checked_block;
};
