#include "player.h"

#include "game.h"
#include "world.h"
#include "input.h"
#include "debug_ui.h"
#include "console.h"

#include "simple_draw.h"

// TODO@ Bug with movement when sliding

namespace {
    constexpr float P_ACCELERATION = 35.0f;
    constexpr float P_DECELERATION = 18.0f;
    constexpr float P_MAX_SPEED        = 5.0f;
    constexpr float P_SPRINT_MAX_SPEED = 212.0f; // @todo
    constexpr float P_JUMP_FORCE   = 8.5f;
    constexpr float P_GRAVITY      = -9.81f * 2.0f;

    constexpr float P_GROUND_COLLIDER_HEIGHT = 0.2f;
    constexpr vec3  P_COLLIDER_SIZE = vec3{ 0.6f, 1.85f, 0.6f };
    constexpr float P_HEAD_OFFSET_PERC = 0.1f; // Head offset from the top of player size in percents
    static_assert(IN_BOUNDS(P_HEAD_OFFSET_PERC, 0.0f, 1.0f));

    bool g_debug_infinite_jump = false;
    bool g_debug_flying = false;
}

Player::Player(void) {
    m_position = vec3::zero();
    m_velocity = vec3::zero();
    m_is_sprinting = false;
    
    Console::set_command("jumper", { CONSOLE_COMMAND_LAMBDA {
            BOOL_TOGGLE(g_debug_infinite_jump);
        }
    });

    Console::set_command("fly", { CONSOLE_COMMAND_LAMBDA {
            BOOL_TOGGLE(g_debug_flying);
        }
    });
}

Player::~Player(void) {
}

bool check_aabb_3d(vec3 pos_1, vec3 size_1, vec3 pos_2, vec3 size_2) {
    vec3 mink_min   = pos_1 - size_2 * 0.5f;
    vec3 mink_max   = pos_1 + size_1 + size_2 * 0.5f;
    vec3 mink_point = pos_2 + size_2 * 0.5f;

    return (
        (mink_point.x >= mink_min.x && mink_point.x <= mink_max.x) &&
        (mink_point.y >= mink_min.y && mink_point.y <= mink_max.y) &&
        (mink_point.z >= mink_min.z && mink_point.z <= mink_max.z)
    );
}

bool Player::check_if_collides_with_any_block(World &world, vec3 position, vec3 size) {
    WorldPosition min;
    WorldPosition max;
    calc_overlapping_blocks(position, size, min, max);

    for(int32_t block_y = min.block.y; block_y <= max.block.y; ++block_y) {
        for(int32_t block_x = min.block.x; block_x <= max.block.x; ++block_x) {
            for(int32_t block_z = min.block.z; block_z <= max.block.z; ++block_z) {
                WorldPosition block_p = WorldPosition::from_block({ block_x, block_y, block_z });

                Block *block = world.get_block(block_p.block);
                if(block == NULL) {
                    /* Block is from chunk that doesn't exist */
                    continue;
                }

                if(check_aabb_3d(position, size, block_p.real, { 1.0f, 1.0f, 1.0f }) && block->is_solid()) {
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

    if(!Console::is_open() && input.button_is_down(Button::LEFT)) {
        m_camera.rotate_by(-input.mouse_rel_y(), input.mouse_rel_x(), delta_time);
    }

    const vec3 dir_xz   = m_camera.calc_direction_xz();
    const vec3 dir_side = m_camera.calc_direction_side();
    const bool is_grounded_now = m_is_grounded;

    m_is_sprinting = false;

    int32_t move = 0, move_side = 0;
    if(!Console::is_open()) {
        if(input.key_is_down(Key::W)) {
            move += 1;
        }
        if(input.key_is_down(Key::S)) {
            move -= 1;
        }
        if(input.key_is_down(Key::D)) {
            move_side += 1;
        }
        if(input.key_is_down(Key::A)) {
            move_side -= 1;
        }

        if(g_debug_flying) {
            m_velocity.y = 0;
            if(input.key_is_down(Key::SPACE)) {
                m_velocity.y = 100.0f;
            }
            if(input.key_is_down(Key::LEFT_SHIFT)) {
                m_velocity.y = -100.0f;
            }
            if(input.key_is_down(Key::LEFT_CTRL)) {
                m_is_sprinting = true;
            }

        } else {
            if(input.key_pressed(Key::SPACE) && (is_grounded_now || g_debug_infinite_jump)) {
                m_velocity.y = P_JUMP_FORCE;
            }
        }
    }

    const bool move_input = move != 0 || move_side != 0;
    vec3 dir = move * dir_xz + move_side * dir_side;
    if(move_input) {
        dir = vec3::normalize(dir);
    }

    if(!g_debug_flying) {
        m_velocity.y += P_GRAVITY * delta_time;
    }

    if(move_input) {
        m_velocity += vec3{ 
            .x = dir.x * P_ACCELERATION * delta_time,
            .z = dir.z * P_ACCELERATION * delta_time,
        };
    } else {
        const float deceleration = P_DECELERATION * delta_time;
        const float epsilon = 0.1f;

        vec2 vel_xz = { m_velocity.x, m_velocity.z };
        vec2 unit_v = vec2::normalize(vel_xz);
        float vel_len = vec2::length(vel_xz);

        if(vel_len > epsilon) {
            m_velocity.x -= deceleration * unit_v.x;
            m_velocity.z -= deceleration * unit_v.y;
        } else {
            m_velocity.x = 0.0f;
            m_velocity.z = 0.0f;
        }

        if(vec2::dot(vel_xz, m_velocity.get_xz()) < 0.0f) {
            m_velocity.x = 0.0f;
            m_velocity.z = 0.0f;
        }
    }

    vec2  velocity_xz = { m_velocity.x, m_velocity.z };
    float velocity_xz_len = vec2::length(velocity_xz);
    if(velocity_xz_len && move_input) {
        vec2 velocity_xz_unit = vec2::normalize(velocity_xz);
        clamp_max_v(velocity_xz_len, m_is_sprinting ? P_SPRINT_MAX_SPEED : P_MAX_SPEED);
        velocity_xz = velocity_xz_unit * velocity_xz_len;
        m_velocity.x = velocity_xz.x;
        m_velocity.z = velocity_xz.y;
    }

    this->move_in_xz(world, delta_time);
    this->move_in_y(world, delta_time);

    m_camera.set_position(this->get_position_head());

    const Camera head_camera = this->get_head_camera();
    const vec3 ray_origin = head_camera.get_position();
    const vec3 ray_end    = ray_origin + head_camera.calc_direction() * 8.0f;
    RaycastBlockResult raycast_result = raycast_block(world, ray_origin, ray_end);
    if(raycast_result.found && input.button_pressed(Button::RIGHT)) {
        WorldPosition target = WorldPosition::from_block(raycast_result.block_p.block + vec3i::make(raycast_result.normal));
        Chunk *chunk = world.get_chunk(target.chunk);
        if(chunk) {
            Block *block = chunk->get_block(target.block_rel);
            if(block) {
                block->set_type(BlockType::COBBLESTONE);
                world.queue_chunk_vao_load(chunk->get_coords());
            }
        }
    } else if(raycast_result.found && input.key_pressed_or_repeat(Key::DELETE)) {
        Chunk *chunk = world.get_chunk(raycast_result.block_p.chunk);
        if(chunk) {
            Block *block = chunk->get_block(raycast_result.block_p.block_rel);
            if(block) {
                block->set_type(BlockType::AIR);
                world.queue_chunk_vao_load(chunk->get_coords());
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
    min = WorldPosition::from_block(block_min_p - vec3i{ 1, 1, 1 });
    max = WorldPosition::from_block(block_max_p + vec3i{ 1, 1, 1 });
}

void Player::move_in_xz(World &world, float delta_time) {

    // x and z are interchangable
    auto test_block_edge = [] (float player_pos_x, float player_pos_z, float delta_x, float delta_z, float wall_x, float wall_min_z, float wall_max_z, float &t_min) -> bool {
        if(delta_x != 0.0f) {
            float t_result = (wall_x - player_pos_x) / delta_x;
            if(t_result >= 0.0f && t_result < t_min) {
                float z = player_pos_z + t_result * delta_z;
                if(z >= wall_min_z && z <= wall_max_z) {
                    t_min = t_result - 0.1f;
                    return true;
                }
            }
        }
        return false;
    };

    float t_min = 1.0f;

    uint32_t iter = 0;
    while(t_min > 0.0f && iter++ < 4) {
        vec3 move_delta = vec3{ m_velocity.x, 0.0f, m_velocity.z } * delta_time;
        WorldPosition min, max;
        calc_range_to_check_move(m_position, P_COLLIDER_SIZE, move_delta, min, max);

        _ADD_TO_DEBUG_RANGE(min, max);

        vec3i wall_normal = { };

        for(int32_t block_y = min.block.y; block_y <= max.block.y; ++block_y) {
            for(int32_t block_x = min.block.x; block_x <= max.block.x; ++block_x) {
                for(int32_t block_z = min.block.z; block_z <= max.block.z; ++block_z) {
                    WorldPosition block_p = WorldPosition::from_block({ block_x, block_y, block_z });

                    Block *block = world.get_block(block_p.block);
                    if(block == NULL) {
                        /* Block is from chunk that doesn't exist */
                        continue;
                    }

                    if(!block->is_solid()) {
                        continue;
                    }

                    if(m_position.y >= (block_p.real.y + 1.0f) || (m_position.y + P_COLLIDER_SIZE.y) <= block_p.real.y) {
                        continue;
                    }

                    /* Check collisions with blocks in xz axis as 2d for now... */

                    const vec2 collider_size = { P_COLLIDER_SIZE.x, P_COLLIDER_SIZE.z };
                    const vec2 block_size = { 1.0f, 1.0f };

                    vec2 min_corner = vec2{ block_p.real.x, block_p.real.z } - collider_size * 0.5f;
                    vec2 max_corner = vec2{ block_p.real.x, block_p.real.z } + collider_size * 0.5f + block_size;
                    vec2 player_pos = vec2{ m_position.x, m_position.z }     + collider_size * 0.5f;

                    if(test_block_edge(player_pos.x, player_pos.y, move_delta.x, move_delta.z, min_corner.x, min_corner.y, max_corner.y, t_min)) {
                        wall_normal = { -1, 0, 0 };
                    }

                    if(test_block_edge(player_pos.x, player_pos.y, move_delta.x, move_delta.z, max_corner.x, min_corner.y, max_corner.y, t_min)) {
                        wall_normal = { +1, 0, 0 };
                    }

                    if(test_block_edge(player_pos.y, player_pos.x, move_delta.z, move_delta.x, min_corner.y, min_corner.x, max_corner.x, t_min)) {
                        wall_normal = { 0, 0, -1 };
                    }

                    if(test_block_edge(player_pos.y, player_pos.x, move_delta.z, move_delta.x, max_corner.y, min_corner.x, max_corner.x, t_min)) {
                        wall_normal = { 0, 0, +1 };
                    }
                }
            }
        }

        m_position.x += t_min * move_delta.x;
        m_position.z += t_min * move_delta.z;

        t_min = 1.0f - t_min;

        if(wall_normal.x) {
            m_velocity.x = 0.0f;
        }

        if(wall_normal.z) {
            m_velocity.z = 0.0f;
        }
    }
}

void Player::move_in_y(World &world, float delta_time) {
    vec3 move_delta = vec3{ 0.0f, m_velocity.y, 0.0f } * delta_time;
    vec3 new_position = m_position + move_delta;

    WorldPosition min, max;
    calc_range_to_check_move(m_position, P_COLLIDER_SIZE, move_delta, min, max);

    if(this->check_if_collides_with_any_block(world, new_position, P_COLLIDER_SIZE)) {
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
    vec3 min_pos = real_position_from_block(m_debug_min_checked_block);
    vec3 max_pos = real_position_from_block(m_debug_max_checked_block) + vec3{ 1.0f, 1.0f, 1.0f };

    SimpleDraw::draw_cube_outline(min_pos, max_pos - min_pos, 0.05f, Color{ 0.8f, 0.5f, 0.9f, 0.0f}); 
}

vec3 Player::get_size(void) const {
    return P_COLLIDER_SIZE;
}

void Player::set_position(const vec3 &position) {
    m_position = position;
}

vec3 Player::get_position(void) const {
    return m_position;
}

vec3 Player::get_position_center(void) const {
    return m_position + P_COLLIDER_SIZE * 0.5f;
}

vec3 Player::get_position_head(void) const {
    return {
        .x = m_position.x + P_COLLIDER_SIZE.x * 0.5f,
        .y = m_position.y + P_COLLIDER_SIZE.y * (1.0f - P_HEAD_OFFSET_PERC),
        .z = m_position.z + P_COLLIDER_SIZE.z * 0.5f,
    };
}

Camera &Player::get_head_camera(void) {
    return m_camera;
}

bool Player::is_grounded(void) const {
    return m_is_grounded;
}

vec3 Player::get_velocity(void) const {
    return m_velocity;
}

void Player::get_ground_collider_info(vec3 &pos, vec3 &size) {
    pos = this->get_position() - vec3{ 0.0f, P_GROUND_COLLIDER_HEIGHT * 0.5f, 0.0f  };
    size = { P_COLLIDER_SIZE.x, P_GROUND_COLLIDER_HEIGHT, P_COLLIDER_SIZE.z };
}
