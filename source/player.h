#pragma once

#include "common.h"
#include "math_types.h"
#include "camera.h"
#include "world.h"
#include "world_utils.h"

class Input;
class Game;
class Console;

enum class PlayerMovementMode {
    NORMAL,
    FLYING,
};

class Player {
public:
    CLASS_COPY_DISABLE(Player);

    Player(void) = default;

    /* Initialize player values */
    void initialize(World &world, vec2i spawn_chunk);

    /* Player's update proc */
    void update(Game &game, const Input &input, float delta_time);

    /* Render debug stuff */
    void debug_render(Game &game);

    /* Size of the collider */
    vec3 get_collider_size(void) const;

    /* Origin corner */
    vec3 get_position_origin(void) const;

    /* Center bottom position */
    vec3 get_position(void) const;

    /* Position of the head camera */
    vec3 get_position_head(void) const;

    /* Set origin position */
    void set_position_origin(const vec3 &real);

    /* Set bottom center position */
    void set_position(const vec3 &real);

    /* Get player's current chunk position */
    vec2i get_position_chunk(void);

    /* Get player's head camera */
    Camera get_head_camera(void);

    /* Get player's 3rd person camera */
    Camera get_3rd_person_camera(float distance);

    /* If moved chunks last update() */
    bool moved_chunk_last_frame(void) const;

    /* Current velocity */
    vec3 get_velocity(void) const;

    /* Is grounded since last update() */
    bool is_grounded(void) const;

    /* Ground check collider */
    void get_ground_collider_info(vec3 &pos, vec3 &size);

    /* Set player movement mode */
    void set_movement_mode(PlayerMovementMode mode);

    /* Get currently set movement mode */
    PlayerMovementMode get_movement_mode(void);

    /* Currently held block */
    BlockType get_held_block(void);

    /* Currently targeted block */
    RaycastBlockResult get_targeted_block(void);

private:
    bool check_if_collides_with_any_block(World &world, vec3 position, vec3 size);

    void move_in_xz(World &world, float delta_time);
    void move_in_y(World &world, float delta_time);
 
    bool   m_is_grounded;
    vec3   m_velocity;
    vec3   m_position;
    bool   m_is_sprinting;
    Camera m_head_camera;

    BlockType m_held_block;

    bool m_moved_chunk_last_frame;

    PlayerMovementMode m_movement_mode;

    /* Targeted block by player */
    RaycastBlockResult m_targeted_block;

    /* Range of blocks that were checked for collision and stuff this frame, for debug display */
    vec3i m_debug_min_checked_block;
    vec3i m_debug_max_checked_block;
};
