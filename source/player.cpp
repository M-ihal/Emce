#include "player.h"

#include "game.h"
#include "world.h"
#include "input.h"
#include "debug_ui.h"
#include "console.h"

namespace {
    constexpr float P_ACCELERATION = 8.0f;
    constexpr float P_DECELERATION = 0.8f;
    constexpr float P_MAX_SPEED = 10.0f;
    constexpr float P_JUMP_FORCE = 10.0f;
    constexpr float P_GRAVITY = -9.81f;

    constexpr float P_GROUND_COLLIDER_HEIGHT = 0.2f;
    constexpr vec3  P_COLLIDER_SIZE = vec3{ 0.6f, 1.85f, 0.6f };
    constexpr float P_HEAD_OFFSET_PERC = 0.1f; // Head offset from the top of player size in percents
    static_assert(IN_BOUNDS(P_HEAD_OFFSET_PERC, 0.0f, 1.0f));

    bool g_debug_infinite_jump = false;
}

Player::Player(void) {
    m_position = vec3::zero();
    m_velocity = vec3::zero();

    Console::register_command({
        .command = "infjmp",
        .proc = CONSOLE_COMMAND_LAMBDA {
            BOOL_TOGGLE(g_debug_infinite_jump);
        }
    });
}

Player::~Player(void) {
}

static bool check_collision_with_any_block(vec3 position, vec3 size, const World &world) {
    WorldPosition min = WorldPosition::from_real(position);
    WorldPosition max = WorldPosition::from_real(position + size);

    for(int32_t block_y = min.block.y; block_y <= max.block.y; ++block_y) {
        for(int32_t block_x = min.block.x; block_x <= max.block.x; ++block_x) {
            for(int32_t block_z = min.block.z; block_z <= max.block.z; ++block_z) {

                const vec3i   block_abs = { block_x, block_y, block_z };
                WorldPosition block_p = WorldPosition::from_block(block_abs);

                const Block *block = world.get_block(block_abs);
                if(block == NULL) {
                    /* Block is from chunk that doesn't exist */
                    continue;
                }

                if(block->is_solid()) {
                    return true;
                }
            }
        }
    }

    return false;
}

void Player::update(Game &game, const Input &input, float delta_time) {
    World &world = game.get_world();

    if(!Console::is_open() && input.button_is_down(Button::LEFT)) {
        m_camera.rotate_by(-input.mouse_rel_y(), input.mouse_rel_x(), delta_time);
    }

    const vec3 dir_xz   = m_camera.calc_direction_xz();
    const vec3 dir_side = m_camera.calc_direction_side();
    const bool is_grounded_now = this->check_is_grounded(world);

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
        if(input.key_pressed(Key::SPACE) && (is_grounded_now || g_debug_infinite_jump)) {
            m_velocity.y += P_JUMP_FORCE;
        }
    }

    const bool move_input = move != 0 || move_side != 0;
    vec3 dir = move * dir_xz + move_side * dir_side;
    if(move_input) {
        dir = vec3::normalize(dir);
    }

    DebugUI::push_text_right("dir: %.3f, %.3f, %.3f", dir.x, dir.y, dir.z);

    if(move_input) {
        m_velocity += vec3{ 
            .x = dir.x * P_ACCELERATION * delta_time,
            .y = P_GRAVITY * delta_time,
            .z = dir.z * P_ACCELERATION * delta_time,
        };
    } else {
        const float deceleration = P_DECELERATION * delta_time;
        if(m_velocity.x) {
            m_velocity.x += deceleration - SIGN(m_velocity.x);
        } else {
            m_velocity.x = 0.0f;
        }
        if(m_velocity.z) {
            m_velocity.z += deceleration - SIGN(m_velocity.z);
        } else {
            m_velocity.z = 0.0f;
        }
    }

    //
    // MOVE XZ
    //

    vec3 new_position = m_position + vec3{ m_velocity.x, 0.0f, m_velocity.z } * delta_time;
    bool collided = check_collision_with_any_block(new_position, P_COLLIDER_SIZE, world);
    if(!collided) {
        m_position = new_position;
    } else {
        m_velocity.x = 0.0f;
        m_velocity.z = 0.0f;
    }

    //
    // MOVE Y
    //

    new_position = m_position + vec3{ 0.0f, m_velocity.y, 0.0f } * delta_time;
    collided = check_collision_with_any_block(new_position, P_COLLIDER_SIZE, world);
    if(!collided) {
        m_position = new_position;
    } else {
        m_velocity.y = 0.0f;
    }

    DebugUI::push_text_left(" --- Player ---");
    DebugUI::push_text_left("position: %.2f, %.2f, %.2f", m_position.x, m_position.y, m_position.z);
    DebugUI::push_text_left("velocity: %.2f, %.2f, %.2f", m_velocity.x, m_velocity.y, m_velocity.z);
    DebugUI::push_text_left("is_grounded: %s", BOOL_STR(this->check_is_grounded(world)));

    m_camera.set_position(this->get_position_head());
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

bool Player::check_is_grounded(const class World &world) {
    vec3 ground_collider_pos;
    vec3 ground_collider_size;
    this->get_ground_collider_info(ground_collider_pos, ground_collider_size);
    return check_collision_with_any_block(ground_collider_pos, ground_collider_size, world);
}

void Player::get_ground_collider_info(vec3 &pos, vec3 &size) {
    pos = this->get_position() - vec3{ 0.0f, P_GROUND_COLLIDER_HEIGHT * 0.5f, 0.0f  };
    size = { P_COLLIDER_SIZE.x, P_GROUND_COLLIDER_HEIGHT, P_COLLIDER_SIZE.z };
}
