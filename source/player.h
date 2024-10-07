#pragma once

#include "common.h"
#include "math_types.h"
#include "camera.h"

class Player {
public:
    CLASS_COPY_DISABLE(Player);

    Player(void);
    ~Player(void);

    void update(class Game &game, const class Input &input, float delta_time);

    void debug_render(class Game &game);

    vec3 get_size(void) const;
    void set_position(const vec3 &position);
    vec3 get_position(void) const;
    vec3 get_position_center(void) const;
    vec3 get_position_head(void) const;
    Camera       &get_head_camera(void);
    const Camera &get_head_camera(void) const;

    vec3 get_velocity(void) const;

    /* @todo Slow procedure */
    bool check_is_grounded(const class World &world);
    void get_ground_collider_info(vec3 &pos, vec3 &size);

private:
    bool check_collision_with_any_block(vec3 position, vec3 size, const World &world);
    
    vec3   m_velocity;
    vec3   m_position;
    bool   m_is_sprinting;
    Camera m_camera;

    /* Range of blocks that were checked for collision and stuff this frame, for debug display */
    vec3i m_debug_min_checked_block;
    vec3i m_debug_max_checked_block;
};
