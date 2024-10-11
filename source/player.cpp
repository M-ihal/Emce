#include "player.h"

#include "game.h"
#include "world.h"
#include "input.h"
#include "debug_ui.h"
#include "console.h"

#include "simple_draw.h"

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
    
    Console::register_command({
        .command = "infjmp",
        .proc = CONSOLE_COMMAND_LAMBDA {
            BOOL_TOGGLE(g_debug_infinite_jump);
        }
    });

    Console::register_command({
        .command = "fly",
        .proc = CONSOLE_COMMAND_LAMBDA {
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

bool Player::check_collision_with_any_block(vec3 position, vec3 size, World &world) {
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
    const bool is_grounded_now = this->check_is_grounded(world);

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
        /*if(input.key_is_down(Key::LEFT_SHIFT)) {
            m_is_sprinting = true;
        }*/
    }

    const bool move_input = move != 0 || move_side != 0;
    vec3 dir = move * dir_xz + move_side * dir_side;
    if(move_input) {
        dir = vec3::normalize(dir);
    }

    DebugUI::push_text_right("dir: %.3f, %.3f, %.3f", dir.x, dir.y, dir.z);

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
#if 0
        vec2 velocity_xz_unit = vec2::normalize({ dir.x, dir.z });
#else
        vec2 velocity_xz_unit = vec2::normalize(velocity_xz);
#endif
        clamp_max_v(velocity_xz_len, m_is_sprinting ? P_SPRINT_MAX_SPEED : P_MAX_SPEED);
        velocity_xz = velocity_xz_unit * velocity_xz_len;
        m_velocity.x = velocity_xz.x;
        m_velocity.z = velocity_xz.y;
    }

    auto add_move_range_to_debug = [&] (WorldPosition &min, WorldPosition &max) {
        m_debug_min_checked_block.x = MIN(m_debug_min_checked_block.x, min.block.x);
        m_debug_min_checked_block.y = MIN(m_debug_min_checked_block.y, min.block.y);
        m_debug_min_checked_block.z = MIN(m_debug_min_checked_block.z, min.block.z);
        m_debug_max_checked_block.x = MAX(m_debug_max_checked_block.x, max.block.x);
        m_debug_max_checked_block.y = MAX(m_debug_max_checked_block.y, max.block.y);
        m_debug_max_checked_block.z = MAX(m_debug_max_checked_block.z, max.block.z);
    };

    //
    // MOVE XZ
    //

    vec3 new_position = m_position + vec3{ m_velocity.x, 0.0f, m_velocity.z } * delta_time;

    WorldPosition min, max;
    calc_overlapping_blocks(new_position, P_COLLIDER_SIZE, min, max);
    bool collided = check_collision_with_any_block(new_position, P_COLLIDER_SIZE, world);
    if(!collided) {
        m_position = new_position;
    } else {
        m_velocity.x = 0.0f;
        m_velocity.z = 0.0f;
    }

    add_move_range_to_debug(min, max);


    //
    // MOVE Y
    //

    new_position = m_position + vec3{ 0.0f, m_velocity.y, 0.0f } * delta_time;

    calc_overlapping_blocks(new_position, P_COLLIDER_SIZE, min, max);
    collided = check_collision_with_any_block(new_position, P_COLLIDER_SIZE, world);
    if(!collided) {
        m_position = new_position;
    } else {
        m_velocity.y = 0.0f;
    }

    add_move_range_to_debug(min, max);

    DebugUI::push_text_left(" --- Player ---");
    DebugUI::push_text_left("position: %.2f, %.2f, %.2f", m_position.x, m_position.y, m_position.z);
    DebugUI::push_text_left("velocity: %.2f, %.2f, %.2f", m_velocity.x, m_velocity.y, m_velocity.z);
    DebugUI::push_text_left("xz speed: %.3f", vec2::length(m_velocity.get_xz()));
    DebugUI::push_text_left("is_grounded: %s", BOOL_STR(this->check_is_grounded(world)));

    m_camera.set_position(this->get_position_head());
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

const Camera &Player::get_head_camera(void) const {
    return m_camera;
}

vec3 Player::get_velocity(void) const {
    return m_velocity;
}

bool Player::check_is_grounded(World &world) {
    vec3 ground_collider_pos;
    vec3 ground_collider_size;
    this->get_ground_collider_info(ground_collider_pos, ground_collider_size);

    return check_collision_with_any_block(ground_collider_pos, ground_collider_size, world);
}

void Player::get_ground_collider_info(vec3 &pos, vec3 &size) {
    pos = this->get_position() - vec3{ 0.0f, P_GROUND_COLLIDER_HEIGHT * 0.5f, 0.0f  };
    size = { P_COLLIDER_SIZE.x, P_GROUND_COLLIDER_HEIGHT, P_COLLIDER_SIZE.z };
}
