#include "player.h"

#include "game.h"
#include "world.h"
#include "input.h"

namespace {
    constexpr vec3  s_player_size = vec3{ 0.8f, 1.8f, 0.8f };
    constexpr float s_player_head_offset_perc = 0.1f; // Head offset from the top of player size in percents
    static_assert(IN_BOUNDS(s_player_head_offset_perc, 0.0f, 1.0f));
}

Player::Player(void) {
    m_position = vec3::zero();
}

Player::~Player(void) {

}

void Player::update(Game &game, const Input &input, float delta_time) {
    vec3 next_position = m_position;
    next_position.y -= delta_time * 1.4f;
    
    WorldPosition world_p = WorldPosition::from_real(next_position);
    Chunk *chunk = game.get_world().get_chunk(world_p.chunk);
    if(chunk) {
        Block &block = chunk->get_block(world_p.block_rel);
        if(!block.is_of_type(BlockType::AIR)) {
            return;
        }
    }

    m_position = next_position;
}

vec3 Player::get_size(void) const {
    return s_player_size;
}

void Player::set_position(const vec3 &position) {
    m_position = position;
}

vec3 Player::get_position(void) const {
    return m_position;
}

vec3 Player::get_position_center(void) const {
    return m_position + s_player_size * 0.5f;
}

vec3 Player::get_position_head(void) const {
    return {
        .x = m_position.x + s_player_size.x * 0.5f,
        .y = m_position.y + s_player_size.y * (1.0f - s_player_head_offset_perc),
        .z = m_position.z + s_player_size.z * 0.5f,
    };
}
