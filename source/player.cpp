#include "player.h"

#include "game.h"
#include "world.h"
#include "input.h"
#include "console.h"

#include "simple_draw.h"

namespace {
    constexpr float PLAYER_ACCELERATION           = 35.0f;
    constexpr float PLAYER_DECELERATION           = 18.0f;
    constexpr float PLAYER_MAX_SPEED              = 5.0f;
    constexpr float PLAYER_SPRINT_MAX_SPEED       = 212.0f;
    constexpr float PLAYER_JUMP_FORCE             = 8.5f;
    constexpr float PLAYER_GRAVITY                =-9.81f * 2.0f;
    constexpr float PLAYER_GROUND_COLLIDER_HEIGHT = 0.2f;
    constexpr vec3  PLAYER_COLLIDER_SIZE          = vec3{ 0.6f, 1.85f, 0.6f };
    constexpr float PLAYER_HEAD_OFFSET_PERC       = 0.1f; // Offset percentage
    constexpr float PLAYER_RAYCAST_DIST           = 16.0f;
}

void Player::setup_player(World &world, vec2i spawn_chunk) {
    m_velocity     = vec3::zero();
    m_is_sprinting = false;
    m_held_block   = BlockType::TREE_LOG;
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
        if(block != BlockType::_INVALID && get_block_flags(block) & IS_SOLID) {
            spawn_y++;
            break;
        }
        spawn_y--;
    }
    this->set_position_origin(real_position_from_block({ spawn_x, spawn_y, spawn_z }));
}

bool check_aabb_3d(vec3 pos_1, vec3 size_1, vec3 pos_2, vec3 size_2) {
    vec3 mink_min   = pos_1 - size_2 * 0.5f;
    vec3 mink_max   = pos_1 + size_1 + size_2 * 0.5f;
    vec3 mink_point = pos_2 + size_2 * 0.5f;

    return ((mink_point.x >= mink_min.x && mink_point.x <= mink_max.x) &&
            (mink_point.y >= mink_min.y && mink_point.y <= mink_max.y) &&
            (mink_point.z >= mink_min.z && mink_point.z <= mink_max.z));
}

bool Player::check_if_collides_with_any_block(World &world, vec3 position, vec3 size) {
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

                if(check_aabb_3d(position, size, block_p.real, { 1.0f, 1.0f, 1.0f }) && get_block_flags(block) & IS_SOLID) {
                    return true;
                }
            }
        }
    }
    return false;
}

void Player::update(Game &game, const Input &input, float delta_time) {
    World &world = game.get_world();

    m_debug_min_checked_block = vec3i{ 1000000, 1000000, 1000000 };
    m_debug_max_checked_block = vec3i{ -1000000, -1000000, -1000000 };

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
        } while(!(get_block_flags(m_held_block) & IS_PLACABLE));
    }

    const bool can_jump = m_is_grounded;
    const vec3 dir_xz = m_head_camera.calc_direction_xz();
    const vec3 dir_side = m_head_camera.calc_direction_side();

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
    const vec3 move_dir = move_input ? vec3::normalize(move_forw * dir_xz + move_side * dir_side) : vec3::zero();

    /* Apply acceleration or decceleration */
    if(move_input) {
        m_velocity += vec3{ 
            .x = move_dir.x * PLAYER_ACCELERATION * delta_time,
            .z = move_dir.z * PLAYER_ACCELERATION * delta_time,
        };
    } else {
        const float deceleration = PLAYER_DECELERATION * delta_time;
        const float epsilon = 0.1f;

        vec2  vel_xz = { m_velocity.x, m_velocity.z };
        vec2  vel_xz_norm = vec2::normalize(vel_xz);
        float vel_xz_len = vec2::length(vel_xz);

        if(vel_xz_len > epsilon) {
            m_velocity.x -= deceleration * vel_xz_norm.x;
            m_velocity.z -= deceleration * vel_xz_norm.y;
        } else {
            m_velocity.x = 0.0f;
            m_velocity.z = 0.0f;
        }

        if(vec2::dot(vel_xz, m_velocity.get_xz()) < 0.0f) {
            m_velocity.x = 0.0f;
            m_velocity.z = 0.0f;
        }
    }

    /* Clamp speed to max */
    vec2  vel_xz = { m_velocity.x, m_velocity.z };
    float vel_xz_len = vec2::length(vel_xz);
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
        vec3 move_delta = m_velocity * delta_time;
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
    const vec3   ray_origin  = head_camera.get_position();
    const vec3   ray_end     = ray_origin + head_camera.calc_direction() * PLAYER_RAYCAST_DIST;

    /* Target block and/or put/destroy it */

    m_targeted_block = raycast_block(world, ray_origin, ray_end);

    if(m_targeted_block.found) {
        Chunk *target_chunk = NULL;
        BlockType target = world.get_block(m_targeted_block.block_p.block, &target_chunk);
        if(target != BlockType::_INVALID) {

            if(block_place) {
                WorldPosition place_block_p = WorldPosition::from_block(m_targeted_block.block_p.block + get_block_side_dir(m_targeted_block.side));
                Chunk *place_block_chunk = world.get_chunk(place_block_p.chunk);
                if(place_block_chunk != NULL) {
                    bool would_collide = check_aabb_3d(this->get_position_origin(), PLAYER_COLLIDER_SIZE, place_block_p.real, vec3::make(1));
                    if(!would_collide) {
                        place_block_chunk->set_block(place_block_p.block_rel, m_held_block);

                        // @TODO : If no bordering chunks will crash !!
                        vec2i chunk_xz = m_targeted_block.block_p.chunk;
                        world.gen_chunk_mesh_imm(chunk_xz);

                        /* Update neighbouring chunk/s if block was bordering them */
                        if(is_block_on_chunk_edge(m_targeted_block.block_p.block_rel, BlockSide::X_NEG)) {
                            world.gen_chunk_mesh_imm(chunk_xz + vec2i{ -1, 0 });
                        }
                        if(is_block_on_chunk_edge(m_targeted_block.block_p.block_rel, BlockSide::X_POS)) {
                            world.gen_chunk_mesh_imm(chunk_xz + vec2i{ 1, 0 });
                        }
                        if(is_block_on_chunk_edge(m_targeted_block.block_p.block_rel, BlockSide::Z_NEG)) {
                            world.gen_chunk_mesh_imm(chunk_xz + vec2i{ 0, -1 });
                        }
                        if(is_block_on_chunk_edge(m_targeted_block.block_p.block_rel, BlockSide::Z_POS)) {
                            world.gen_chunk_mesh_imm(chunk_xz + vec2i{ 0, +1 });
                        }
                    }
                }
            }

            if(block_destroy) {
                target_chunk->set_block(m_targeted_block.block_p.block_rel, BlockType::AIR);

                // @TODO : If no bordering chunks will crash !!
                
                vec2i chunk_xz = m_targeted_block.block_p.chunk;
                world.gen_chunk_mesh_imm(chunk_xz);

                /* Update neighbouring chunk/s if block was bordering them */
                if(is_block_on_chunk_edge(m_targeted_block.block_p.block_rel, BlockSide::X_NEG)) {
                    world.gen_chunk_mesh_imm(chunk_xz + vec2i{ -1, 0 });
                }
                if(is_block_on_chunk_edge(m_targeted_block.block_p.block_rel, BlockSide::X_POS)) {
                    world.gen_chunk_mesh_imm(chunk_xz + vec2i{ 1, 0 });
                }
                if(is_block_on_chunk_edge(m_targeted_block.block_p.block_rel, BlockSide::Z_NEG)) {
                    world.gen_chunk_mesh_imm(chunk_xz + vec2i{ 0, -1 });
                }
                if(is_block_on_chunk_edge(m_targeted_block.block_p.block_rel, BlockSide::Z_POS)) {
                    world.gen_chunk_mesh_imm(chunk_xz + vec2i{ 0, +1 });
                }
            }
        }
    }
}

#define _ADD_TO_DEBUG_RANGE(min, max)\
    m_debug_min_checked_block.x = MIN(m_debug_min_checked_block.x, min.block.x);\
    m_debug_min_checked_block.y = MIN(m_debug_min_checked_block.y, min.block.y);\
    m_debug_min_checked_block.z = MIN(m_debug_min_checked_block.z, min.block.z);\
    m_debug_max_checked_block.x = MAX(m_debug_max_checked_block.x, max.block.x);\
    m_debug_max_checked_block.y = MAX(m_debug_max_checked_block.y, max.block.y);\
    m_debug_max_checked_block.z = MAX(m_debug_max_checked_block.z, max.block.z);

static void calc_range_to_check_move(vec3 pos, vec3 size, vec3 move_delta, WorldPosition &min, WorldPosition &max) {
    vec3i block_min_p = block_position_from_real(pos + move_delta);
    vec3i block_max_p = block_position_from_real(pos + move_delta + size);
    min = WorldPosition::from_block(block_min_p); 
    max = WorldPosition::from_block(block_max_p);
}

static bool test_block_edge(float pos_x, float pos_z, float delta_x, float delta_z, float wall_x, float wall_min_z, float wall_max_z, float &t_closest) {
    if(delta_x != 0.0f) {
        float t_result = (wall_x - pos_x) / delta_x;
        if(t_result >= 0.0f && t_result < t_closest) {
            float z = pos_z + t_result * delta_z;
            if(z >= wall_min_z && z <= wall_max_z) {
                const float eps = 0.01f;
                t_closest = t_result - eps;
                return true;
            }
        }
    }
    return false;
}

/* @TODO: */
void Player::move_in_xz(World &world, float delta_time) {
    float t_remaining  = 1.0f;
    uint32_t iteration = 0;
    vec3 velocity = m_velocity;

    while(t_remaining > 0.0f && iteration++ < 4) {
        vec3 move_delta = vec3{ velocity.x, 0.0f, velocity.z } * delta_time;
        WorldPosition min, max;
        calc_range_to_check_move(m_position, PLAYER_COLLIDER_SIZE, move_delta, min, max);

        _ADD_TO_DEBUG_RANGE(min, max);

        vec3i wall_normal = { };
        float t_closest = t_remaining;

        for(int32_t block_y = min.block.y; block_y <= max.block.y; ++block_y) {
            for(int32_t block_x = min.block.x; block_x <= max.block.x; ++block_x) {
                for(int32_t block_z = min.block.z; block_z <= max.block.z; ++block_z) {
                    WorldPosition block_p = WorldPosition::from_block({ block_x, block_y, block_z });

                    BlockType block = world.get_block(block_p.block);
                    if(block == BlockType::_INVALID) {
                        /* Block is from chunk that doesn't exist */
                        continue;
                    }

                    if(!(get_block_flags(block) & IS_SOLID)) {
                        continue;
                    }

                    if(m_position.y >= (block_p.real.y + 1.0f) || (m_position.y + PLAYER_COLLIDER_SIZE.y) <= block_p.real.y) {
                        continue;
                    }

                    /* Check collisions with blocks in xz axis as 2d... */

                    const vec2 collider_size = { PLAYER_COLLIDER_SIZE.x, PLAYER_COLLIDER_SIZE.z };
                    const vec2 block_size = { 1.0f, 1.0f };

                    vec2 min_corner = vec2{ block_p.real.x, block_p.real.z } - collider_size * 0.5f;
                    vec2 max_corner = vec2{ block_p.real.x, block_p.real.z } + collider_size * 0.5f + block_size;
                    vec2 player_pos = vec2{ m_position.x, m_position.z }     + collider_size * 0.5f;

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

void Player::move_in_y(World &world, float delta_time) {
    vec3 move_delta = vec3{ 0.0f, m_velocity.y, 0.0f } * delta_time;
    vec3 new_position = m_position + move_delta;

    WorldPosition min, max;
    calc_range_to_check_move(m_position, PLAYER_COLLIDER_SIZE, move_delta, min, max);

    if(this->check_if_collides_with_any_block(world, new_position, PLAYER_COLLIDER_SIZE)) {
        m_velocity.y = 0.0f;
    } else {
        m_position = new_position;
    }

    vec3 ground_collider_pos;
    vec3 ground_collider_size;
    this->get_ground_collider_info(ground_collider_pos, ground_collider_size);
    m_is_grounded = this->check_if_collides_with_any_block(world, ground_collider_pos, ground_collider_size);

    _ADD_TO_DEBUG_RANGE(min, max);
}

void Player::debug_render(class Game &game) {
    /* Collider */
    const vec3 collider_color = { 0.9f, 0.9f, 0.6f };
    SimpleDraw::draw_cube_outline(this->get_position_origin(), this->get_collider_size(), 1.0f / 32.0f, collider_color);

    /* Ground check collider */
    const vec3 ground_collider_color = m_is_grounded ? vec3{ 0.0f, 1.0f, 0.0f } : vec3{ 1.0f, 0.0f, 0.0f };
    vec3 ground_collider_pos;
    vec3 ground_collider_size;
    this->get_ground_collider_info(ground_collider_pos, ground_collider_size);
    SimpleDraw::draw_cube_outline(ground_collider_pos, ground_collider_size, 1.0f / 32.0f, ground_collider_color);

    /* Velocity lines */
    SimpleDraw::draw_line(this->get_position(), this->get_position() + vec3{ m_velocity.x, 0.0f, m_velocity.z }, 2.0f, { 1.0f, 0.0f, 1.0f });
    SimpleDraw::draw_line(this->get_position(), this->get_position() + vec3{ 0.0f, m_velocity.y, 0.0f }, 2.0f, { 0.0f, 1.0f, 0.0f });

    /* Blocks checked when checking collisions */
    vec3 min_pos = real_position_from_block(m_debug_min_checked_block);
    vec3 max_pos = real_position_from_block(m_debug_max_checked_block) + vec3{ 1.0f, 1.0f, 1.0f };
    SimpleDraw::draw_cube_outline(min_pos, max_pos - min_pos, 0.05f, { 0.8f, 0.5f, 0.9f }); 
}

vec3 Player::get_collider_size(void) const {
    return PLAYER_COLLIDER_SIZE;
}

vec3 Player::get_position_origin(void) const {
    return m_position;
}

vec3 Player::get_position(void) const {
    return m_position + vec3{ PLAYER_COLLIDER_SIZE.x * 0.5f, 0.0f, PLAYER_COLLIDER_SIZE.z * 0.5f };
}

vec3 Player::get_position_head(void) const {
    return {
        .x = m_position.x + PLAYER_COLLIDER_SIZE.x * 0.5f,
        .y = m_position.y + PLAYER_COLLIDER_SIZE.y * (1.0f - PLAYER_HEAD_OFFSET_PERC),
        .z = m_position.z + PLAYER_COLLIDER_SIZE.z * 0.5f,
    };
}

void Player::set_position_origin(const vec3 &real) {
    m_position = real;
}

void Player::set_position(const vec3 &real) {
    m_position = real - vec3{ PLAYER_COLLIDER_SIZE.x * 0.5f, 0.0f, PLAYER_COLLIDER_SIZE.z * 0.5f };
}

vec2i Player::get_position_chunk(void) {
    return chunk_position_from_real(this->get_position());
}

Camera Player::get_head_camera(void) {
    return m_head_camera;
}

Camera Player::get_3rd_person_camera(float distance) {
    clamp_min_v(distance, 0.0f);

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

vec3 Player::get_velocity(void) const {
    return m_velocity;
}

void Player::get_ground_collider_info(vec3 &pos, vec3 &size) {
    pos = this->get_position_origin() - vec3{ 0.0f, PLAYER_GROUND_COLLIDER_HEIGHT * 0.5f, 0.0f  };
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
