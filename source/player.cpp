#include "player.h"

#include "game.h"
#include "world.h"
#include "world_utils.h"
#include "input.h"
#include "console.h"

namespace {
    constexpr double PLAYER_ACCELERATION           = 100; // 35.0;
    constexpr double PLAYER_DECELERATION           = 18.0;
    constexpr double PLAYER_MAX_SPEED              = 5.0;
    constexpr double PLAYER_SPRINT_MAX_SPEED       = 4000; // 212.0;
    constexpr double PLAYER_JUMP_FORCE             = 8.5;
    constexpr double PLAYER_GRAVITY                =-9.81 * 2.0;
    constexpr double PLAYER_GROUND_COLLIDER_HEIGHT = 0.2;
    constexpr vec3d  PLAYER_COLLIDER_SIZE          = vec3d{ 0.6, 1.85, 0.6 };
    constexpr double PLAYER_HEAD_OFFSET_PERC       = 0.1; // Offset percentage
    constexpr double PLAYER_RAYCAST_DIST           = 16.0;
}

void Player::setup_player(World &world, vec2i spawn_chunk) {
    m_velocity     = vec3d::zero();
    m_is_sprinting = false;
    m_held_block   = BlockType::TREE_OAK_LOG;
    m_head_camera.initialize();
    m_movement_mode = PlayerMovementMode::NORMAL;
    m_moved_chunk_last_frame = true;

    const vec3i spawn_block_rel = { CHUNK_SIZE_X / 2, 0, CHUNK_SIZE_Z / 2 };
    const vec2i spawn_chunk_block = vec2i::make_xz(block_position_from_relative(spawn_block_rel, spawn_chunk));

    /* Figure player spawn position */
    int32_t spawn_x = spawn_chunk_block.x;
    int32_t spawn_z = spawn_chunk_block.y;
    int32_t spawn_y = CHUNK_SIZE_Y;

    /* Find 2 not solid blocks to set player position */
    while(spawn_y >= 0) {
        BlockType block = world.get_block({ spawn_x, spawn_y, spawn_z });
        if(block != BlockType::_INVALID && get_block_type_flags(block) & IS_SOLID) {
            spawn_y++;
            break;
        }
        spawn_y--;
    }
    this->set_position_origin(real_position_from_block({ spawn_x, spawn_y, spawn_z }));
}

bool check_aabb_3d(vec3d pos_1, vec3d size_1, vec3d pos_2, vec3d size_2) {
    vec3d mink_min   = pos_1 - size_2 * 0.5f;
    vec3d mink_max   = pos_1 + size_1 + size_2 * 0.5f;
    vec3d mink_point = pos_2 + size_2 * 0.5f;

    return ((mink_point.x >= mink_min.x && mink_point.x <= mink_max.x) &&
            (mink_point.y >= mink_min.y && mink_point.y <= mink_max.y) &&
            (mink_point.z >= mink_min.z && mink_point.z <= mink_max.z));
}

bool Player::check_if_collides_with_any_block(World &world, vec3d position, vec3d size) {
    WorldPosition min;
    WorldPosition max;
    calc_overlapping_blocks(position, size, min, max);

    for(int32_t block_y = min.block.y; block_y <= max.block.y; ++block_y) {
        for(int32_t block_x = min.block.x; block_x <= max.block.x; ++block_x) {
            for(int32_t block_z = min.block.z; block_z <= max.block.z; ++block_z) {
                WorldPosition block_p = WorldPosition::from_block({ block_x, block_y, block_z });

                BlockType block = world.get_block(block_p.block);
                if(block == BlockType::_INVALID) {
                    /* Block is from chunk that doesn't exist */
                    continue;
                }

                if(check_aabb_3d(position, size, block_p.real, { 1.0f, 1.0f, 1.0f }) && get_block_type_flags(block) & IS_SOLID) {
                    return true;
                }
            }
        }
    }
    return false;
}

void Player::update(Game &game, const Input &input, double delta_time) {
    World &world = game.get_world();

    debug_min_checked_block = vec3i{ INT32_MAX, INT32_MAX, INT32_MAX };
    debug_max_checked_block = vec3i{ INT32_MIN, INT32_MIN, INT32_MIN };

    /* Can get input for update */
    const bool can_get_input = !game.get_console().is_open();

    int32_t rotate_v = 0;
    int32_t rotate_h = 0;
    int32_t held_delta = 0;

    int32_t move_forw = 0;
    int32_t move_side = 0;
    bool    move_fast = false;
    bool    try_jump  = false;

    int32_t fly_dir = 0;
    bool    fly_fast = false;

    bool block_destroy = false;
    bool block_place   = false;

    if(can_get_input) {
        rotate_v = -input.mouse_rel_y();
        rotate_h = input.mouse_rel_x();
        held_delta = SIGN(input.scroll_move());

        if(input.key_is_down(Key::W)) { move_forw += 1; }
        if(input.key_is_down(Key::S)) { move_forw -= 1; }
        if(input.key_is_down(Key::D)) { move_side += 1; }
        if(input.key_is_down(Key::A)) { move_side -= 1; }
        if(input.key_pressed(Key::SPACE)) { try_jump = true; }

        if(input.key_is_down(Key::SPACE))      { fly_dir += 1; }
        if(input.key_is_down(Key::LEFT_SHIFT)) { fly_dir -= 1; }
        if(input.key_is_down(Key::LEFT_CTRL))  { fly_fast = true; }

        if(input.button_pressed(Button::LEFT))  { block_destroy = true; }
        if(input.button_pressed(Button::RIGHT)) { block_place = true; }
    }

    /* Rotate camera */
    if(rotate_v != 0 || rotate_h != 0) {
        m_head_camera.rotate_by(rotate_v, rotate_h, delta_time);
    }

    /* Change held block */
    if(held_delta != 0) {
        do {
            int32_t held_block = (int32_t)m_held_block;
            held_block += held_delta;
            if(held_block >= (int32_t)BlockType::_COUNT) {
                held_block = 0;
            } else if(held_block < 0) {
                held_block = (int32_t)BlockType::_COUNT - 1;
            }
            m_held_block = (BlockType)held_block;
        } while(!(get_block_type_flags(m_held_block) & IS_PLACABLE));
    }

    const bool can_jump = m_is_grounded;
    const vec3d dir_xz = m_head_camera.calc_direction_xz();
    const vec3d dir_side = m_head_camera.calc_direction_side();

    if(m_movement_mode == PlayerMovementMode::FLYING) {
        m_is_sprinting = fly_fast;
        m_velocity.y   = fly_dir * 100.0f;
    } else {
        m_is_sprinting = move_fast;
        if(try_jump && can_jump) {
            m_velocity.y = PLAYER_JUMP_FORCE;
        }

        m_velocity.y += PLAYER_GRAVITY * delta_time;
    }

    const bool move_input = move_forw || move_side;
    const vec3d move_dir = move_input ? vec3d::normalize(move_forw * dir_xz + move_side * dir_side) : vec3d::zero();

    /* Apply acceleration or decceleration */
    if(move_input) {
        m_velocity += vec3d{ 
            .x = move_dir.x * PLAYER_ACCELERATION * delta_time,
            .z = move_dir.z * PLAYER_ACCELERATION * delta_time,
        };
    } else {
        const float deceleration = PLAYER_DECELERATION * delta_time;
        const float epsilon = 0.1f;

        vec2  vel_xz = { (float)m_velocity.x, (float)m_velocity.z };
        vec2  vel_xz_norm = vec2::normalize(vel_xz);
        float vel_xz_len = vec2::length(vel_xz);

        if(vel_xz_len > epsilon) {
            if(m_movement_mode != PlayerMovementMode::FLYING) {
                m_velocity.x -= deceleration * vel_xz_norm.x;
                m_velocity.z -= deceleration * vel_xz_norm.y;
            }
        } else {
            m_velocity.x = 0.0f;
            m_velocity.z = 0.0f;
        }

        if(vec2::dot(vel_xz, { (float)m_velocity.x, (float)m_velocity.z }) < 0.0f) {
            m_velocity.x = 0.0f;
            m_velocity.z = 0.0f;
        }
    }

    /* Clamp speed to max */
    vec2  vel_xz = { (float)m_velocity.x, (float)m_velocity.z };
    double vel_xz_len = vec2::length(vel_xz);
    if(vel_xz_len && move_input) {
        vec2 vel_xz_norm = vec2::normalize(vel_xz);
        clamp_max_v(vel_xz_len, m_is_sprinting ? PLAYER_SPRINT_MAX_SPEED : PLAYER_MAX_SPEED);
        vel_xz = vel_xz_norm * vel_xz_len;
        m_velocity.x = vel_xz.x;
        m_velocity.z = vel_xz.y;
    }

    /* Position before the move */
    WorldPosition world_p_last = WorldPosition::from_real(this->get_position());

    /* Do the move */
    if(m_movement_mode == PlayerMovementMode::FLYING) {
        vec3d move_delta = m_velocity * delta_time;
        m_position += move_delta;
    } else {
        this->move_in_xz(world, delta_time);
        this->move_in_y(world, delta_time);
    }
    m_head_camera.set_position(this->get_position_head());

    /* Position after the move */
    WorldPosition world_p_curr = WorldPosition::from_real(this->get_position());

    m_moved_chunk_last_frame = false;
    if(world_p_last.chunk != world_p_curr.chunk) {
        m_moved_chunk_last_frame = true;
    }

    const Camera head_camera = this->get_head_camera();
    const vec3d  ray_origin  = head_camera.get_position();
    const vec3d  ray_end     = ray_origin + head_camera.calc_direction() * PLAYER_RAYCAST_DIST;

    /* Target block and/or put/destroy it */

    m_targeted_block = raycast_block(world, ray_origin, ray_end);

    bool rebuild_mesh = false;
    vec2i chunk_coords;
    vec3i block_rel_changed;

    if(m_targeted_block.found) {
        Chunk *target_chunk = NULL;
        BlockType target = world.get_block(m_targeted_block.block_p.block, &target_chunk);
        if(target != BlockType::_INVALID) {

            if(block_place) {
                WorldPosition place_block_p = WorldPosition::from_block(m_targeted_block.block_p.block + get_block_side_normal(m_targeted_block.side));
                Chunk *place_block_chunk = world.get_chunk(place_block_p.chunk);
                if(place_block_chunk != NULL) {
                    bool would_collide = check_aabb_3d(this->get_position_origin(), PLAYER_COLLIDER_SIZE, place_block_p.real, vec3d::make(1.0));
                    if(!would_collide) {
                        place_block_chunk->set_block(place_block_p.block_rel, m_held_block);

                        rebuild_mesh = true;
                        chunk_coords = place_block_p.chunk;
                        block_rel_changed = place_block_p.block_rel;
                    }
                }
            }

            if(block_destroy) {
                target_chunk->set_block(m_targeted_block.block_p.block_rel, BlockType::AIR);

                rebuild_mesh = true;
                chunk_coords = m_targeted_block.block_p.chunk;
                block_rel_changed = m_targeted_block.block_p.block_rel;
            }
        }
    }

    if(m_targeted_block.found && (block_place || block_destroy)) {
        m_block_action_t = BLOCK_ACTION_ANIM_TIME;
    }

    m_block_action_t -= delta_time;
    if(m_block_action_t <= 0.0) {
        m_block_action_t = 0.0;
    }

    if(rebuild_mesh) {
        world.set_chunk_should_rebuild_mesh(chunk_coords, true);

        /* Update neighbouring chunk/s if block was bordering them */
        if(is_block_on_x_neg_edge(block_rel_changed)) {
            world.set_chunk_should_rebuild_mesh(chunk_coords + vec2i{ -1, 0 }, true);
        }
        if(is_block_on_x_pos_edge(block_rel_changed)) {
            world.set_chunk_should_rebuild_mesh(chunk_coords + vec2i{ 1, 0 }, true);
        }
        if(is_block_on_z_neg_edge(block_rel_changed)) {
            world.set_chunk_should_rebuild_mesh(chunk_coords + vec2i{ 0, -1 }, true);
        }
        if(is_block_on_z_pos_edge(block_rel_changed)) {
            world.set_chunk_should_rebuild_mesh(chunk_coords + vec2i{ 0, +1 }, true);
        }
    }

}

#define _ADD_TO_DEBUG_RANGE(min, max)\
    debug_min_checked_block.x = MIN(debug_min_checked_block.x, min.block.x);\
    debug_min_checked_block.y = MIN(debug_min_checked_block.y, min.block.y);\
    debug_min_checked_block.z = MIN(debug_min_checked_block.z, min.block.z);\
    debug_max_checked_block.x = MAX(debug_max_checked_block.x, max.block.x);\
    debug_max_checked_block.y = MAX(debug_max_checked_block.y, max.block.y);\
    debug_max_checked_block.z = MAX(debug_max_checked_block.z, max.block.z);

static void calc_range_to_check_move(vec3d pos, vec3d size, vec3d move_delta, WorldPosition &min, WorldPosition &max) {
    vec3i block_min_p = block_position_from_real(pos + move_delta);
    vec3i block_max_p = block_position_from_real(pos + move_delta + size);
    min = WorldPosition::from_block(block_min_p); 
    max = WorldPosition::from_block(block_max_p);
}

static bool test_block_edge(double pos_x, double pos_z, double delta_x, double delta_z, double wall_x, double wall_min_z, double wall_max_z, double &t_closest) {
    if(delta_x != 0.0) {
        double t_result = (wall_x - pos_x) / delta_x;
        if(t_result >= 0.0 && t_result < t_closest) {
            double z = pos_z + t_result * delta_z;
            if(z >= wall_min_z && z <= wall_max_z) {
                const double eps = 0.01;
                t_closest = t_result - eps;
                return true;
            }
        }
    }
    return false;
}

/* @TODO: */
void Player::move_in_xz(World &world, double delta_time) {
    double t_remaining  = 1.0f;
    uint32_t iteration = 0;
    vec3d velocity = m_velocity;

    while(t_remaining > 0.0f && iteration++ < 4) {
        vec3d move_delta = vec3d{ velocity.x, 0.0f, velocity.z } * delta_time;
        WorldPosition min, max;
        calc_range_to_check_move(m_position, PLAYER_COLLIDER_SIZE, move_delta, min, max);

        _ADD_TO_DEBUG_RANGE(min, max);

        vec3i wall_normal = { };
        double t_closest = t_remaining;

        for(int32_t block_y = min.block.y; block_y <= max.block.y; ++block_y) {
            for(int32_t block_x = min.block.x; block_x <= max.block.x; ++block_x) {
                for(int32_t block_z = min.block.z; block_z <= max.block.z; ++block_z) {
                    WorldPosition block_p = WorldPosition::from_block({ block_x, block_y, block_z });

                    BlockType block = world.get_block(block_p.block);
                    if(block == BlockType::_INVALID) {
                        /* Block is from chunk that doesn't exist */
                        continue;
                    }

                    if(!(get_block_type_flags(block) & IS_SOLID)) {
                        continue;
                    }

                    if(m_position.y >= (block_p.real.y + 1.0f) || (m_position.y + PLAYER_COLLIDER_SIZE.y) <= block_p.real.y) {
                        continue;
                    }

                    /* Check collisions with blocks in xz axis as 2d... */

                    const vec2d collider_size = { PLAYER_COLLIDER_SIZE.x, PLAYER_COLLIDER_SIZE.z };
                    const vec2d block_size = { 1.0, 1.0 };

                    vec2d min_corner = vec2d{ block_p.real.x, block_p.real.z } - collider_size * 0.5f;
                    vec2d max_corner = vec2d{ block_p.real.x, block_p.real.z } + collider_size * 0.5f + block_size;
                    vec2d player_pos = vec2d{ m_position.x, m_position.z }     + collider_size * 0.5f;

                    if(test_block_edge(player_pos.x, player_pos.y, move_delta.x, move_delta.z, min_corner.x, min_corner.y, max_corner.y, t_closest)) {
                        wall_normal = { -1, 0, 0 };
                    }

                    if(test_block_edge(player_pos.x, player_pos.y, move_delta.x, move_delta.z, max_corner.x, min_corner.y, max_corner.y, t_closest)) {
                        wall_normal = { +1, 0, 0 };
                    }

                    if(test_block_edge(player_pos.y, player_pos.x, move_delta.z, move_delta.x, min_corner.y, min_corner.x, max_corner.x, t_closest)) {
                        wall_normal = { 0, 0, -1 };
                    }

                    if(test_block_edge(player_pos.y, player_pos.x, move_delta.z, move_delta.x, max_corner.y, min_corner.x, max_corner.x, t_closest)) {
                        wall_normal = { 0, 0, +1 };
                    }
                }
            }
        }

        m_position.x += t_closest * move_delta.x;
        m_position.z += t_closest * move_delta.z;
    
        t_remaining -= t_closest;

        if(wall_normal.x) {
            velocity.x = 0.0f;
        }

        if(wall_normal.z) {
            velocity.z = 0.0f;
        }
    }
}

void Player::move_in_y(World &world, double delta_time) {
    vec3d move_delta = vec3d{ 0.0f, m_velocity.y, 0.0f } * delta_time;
    vec3d new_position = m_position + move_delta;

    WorldPosition min, max;
    calc_range_to_check_move(m_position, PLAYER_COLLIDER_SIZE, move_delta, min, max);

    bool collided = false;

    /* Check for collision */ {
        vec3d col_size = PLAYER_COLLIDER_SIZE;
        WorldPosition col_min;
        WorldPosition col_max;
        calc_overlapping_blocks(new_position, col_size, col_min, col_max);

        bool best_found = false;
        WorldPosition best;

        for(int32_t block_y = col_min.block.y; block_y <= col_max.block.y; ++block_y) {
            for(int32_t block_x = col_min.block.x; block_x <= col_max.block.x; ++block_x) {
                for(int32_t block_z = col_min.block.z; block_z <= col_max.block.z; ++block_z) {
                    WorldPosition block_p = WorldPosition::from_block({ block_x, block_y, block_z });

                    BlockType block = world.get_block(block_p.block);
                    if(block == BlockType::_INVALID) {
                        /* Block is from chunk that doesn't exist */
                        continue;
                    }

                    if(check_aabb_3d(new_position, col_size, block_p.real, { 1.0f, 1.0f, 1.0f }) && get_block_type_flags(block) & IS_SOLID) {
                        collided = true;
                        best_found = true;
                        best = block_p;
                        // goto _move_in_y__break_loops;
                    }
                }
            }
        }
    }

    if(collided) {
        m_velocity.y = 0.0f;
    } else {
        m_position = new_position;
    }

    vec3d ground_collider_pos;
    vec3d ground_collider_size;
    this->get_ground_collider_info(ground_collider_pos, ground_collider_size);
    m_is_grounded = this->check_if_collides_with_any_block(world, ground_collider_pos, ground_collider_size);

    _ADD_TO_DEBUG_RANGE(min, max);
}

vec3d Player::get_collider_size(void) const {
    return PLAYER_COLLIDER_SIZE;
}

vec3d Player::get_position_origin(void) const {
    return m_position;
}

vec3d Player::get_position(void) const {
    return m_position + vec3d{ PLAYER_COLLIDER_SIZE.x * 0.5f, 0.0f, PLAYER_COLLIDER_SIZE.z * 0.5f };
}

vec3d Player::get_position_head(void) const {
    return {
        .x = m_position.x + PLAYER_COLLIDER_SIZE.x * 0.5,
        .y = m_position.y + PLAYER_COLLIDER_SIZE.y * (1.0 - PLAYER_HEAD_OFFSET_PERC),
        .z = m_position.z + PLAYER_COLLIDER_SIZE.z * 0.5,
    };
}

void Player::set_position_origin(const vec3d &real) {
    m_position = real;
}

void Player::set_position(const vec3d &real) {
    m_position = real - vec3d{ PLAYER_COLLIDER_SIZE.x * 0.5, 0.0, PLAYER_COLLIDER_SIZE.z * 0.5 };
}

vec2i Player::get_position_chunk(void) {
    return chunk_position_from_real(this->get_position());
}

void Player::set_head_camera_fov(float fov) {
    m_head_camera.set_fov(fov);
}

Camera Player::get_head_camera(void) {
    return m_head_camera;
}

Camera Player::get_3rd_person_camera(double distance) {
    clamp_min_v(distance, 0.0);

    Camera camera = this->get_head_camera();
    camera.set_position(camera.get_position() - camera.calc_direction() * distance);
    return camera;
}

bool Player::moved_chunk_last_frame(void) const {
    return m_moved_chunk_last_frame;
}

bool Player::is_grounded(void) const {
    return m_is_grounded;
}

vec3d Player::get_velocity(void) const {
    return m_velocity;
}

void Player::get_ground_collider_info(vec3d &pos, vec3d &size) {
    pos = this->get_position_origin() - vec3d{ 0.0f, PLAYER_GROUND_COLLIDER_HEIGHT * 0.5, 0.0  };
    size = { 
        PLAYER_COLLIDER_SIZE.x, 
        PLAYER_GROUND_COLLIDER_HEIGHT, 
        PLAYER_COLLIDER_SIZE.z 
    };
}

void Player::set_movement_mode(PlayerMovementMode mode) {
    m_movement_mode = mode;
}

PlayerMovementMode Player::get_movement_mode(void) {
    return m_movement_mode;
}

BlockType Player::get_held_block(void) {
    return m_held_block;
}

RaycastBlockResult Player::get_targeted_block(void) {
    return m_targeted_block;
}
