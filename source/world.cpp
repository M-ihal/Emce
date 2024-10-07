#include "world.h"

#include "game.h"
#include "debug_ui.h"

#include <algorithm>

#include <glew.h>

#define STB_PERLIN_IMPLEMENTATION
#include <stb_perlin.h>

union ChunkKeyInfo {
    uint64_t key;
    struct {
        int32_t x;
        int32_t z;
    };

    static ChunkKeyInfo from_xz(const vec2i &chunk_xz) {
        return { .x = chunk_xz.x, .z = chunk_xz.y };
    }

    static ChunkKeyInfo from_key(uint64_t key) {
        return { .key = key };
    }
};

World::World(const Game *game) {
    m_owner = game;
    m_world_gen_seed = 2137;
    m_chunks.clear();
    m_load_queue.clear();
    m_gen_queue.clear();
    _debug_render_not_fill = false;
}

World::~World(void) {
    this->delete_chunks();
}

void World::initialize_world(int32_t seed) {
    if(this->get_chunk_map_size()) {
        this->delete_chunks();
    }

    m_world_gen_seed = seed;
}

int32_t World::get_seed(void) const {
    return m_world_gen_seed;
}

void World::delete_chunks(void) {
    for(auto const &[key, chunk] : m_chunks) {
        chunk->m_chunk_vao.delete_vao();
        delete chunk;
    }
    m_chunks.clear();
    m_load_queue.clear();
}

void World::queue_chunk_vao_load(vec2i chunk_xz) {
    m_load_queue.push_back(chunk_xz);
    m_should_sort_load_queue = true;
}

void World::process_load_queue(void) {
    if(m_load_queue.empty()) {
        return;
    }

    /* Sorting by distance for now */
    auto sort_func = [&] (const vec2i &c1, const vec2i &c2) -> bool {
        WorldPosition player_chunk_pos = WorldPosition::from_real(m_owner->get_player().get_position());
        vec2i half_chunk_size = vec2i{ 16, 16 } / 2;
        vec2i rel_dist_1 = vec2i::absolute(player_chunk_pos.chunk - c1);
        vec2i rel_dist_2 = vec2i::absolute(player_chunk_pos.chunk - c2);
        
        // stupid
        float dist_1 = vec2::length(vec2::make(rel_dist_1));
        float dist_2 = vec2::length(vec2::make(rel_dist_2));

        return dist_1 > dist_2; 
    };

    if(m_should_sort_load_queue) {
        std::sort(m_load_queue.begin(), m_load_queue.end(), sort_func);
        m_should_sort_load_queue = false;
    }

    while(true) {
        if(!m_load_queue.size()) {
            return;
        }

        auto end = m_load_queue.back();

        WorldPosition player_chunk_pos = WorldPosition::from_real(m_owner->get_player().get_position());
        vec2i rel_dist = vec2i::absolute(player_chunk_pos.chunk - end);
        float dist = vec2::length(vec2::make(rel_dist));
        if(dist > 32.0f) {
            m_load_queue.pop_back();
        } else {
            break;
        }
    }
    
    auto end = m_load_queue.back();
    Chunk *chunk = this->get_chunk(end);

    ChunkVaoGenData vao_data = chunk->gen_vao_data();

    m_gen_queue.push_back(vao_data);
    m_load_queue.pop_back();
}

void World::process_gen_queue(void) {
    if(m_gen_queue.empty()) {
        return;
    }

    ChunkVaoGenData &gen_data = m_gen_queue.back();
    Chunk *chunk = this->get_chunk(gen_data.chunk);
    chunk->gen_vao(gen_data);
    m_gen_queue.pop_back();
}

void World::render_chunks(const Shader &shader, const Texture &atlas) {
    shader.use_program();
    shader.upload_int("u_texture", 0);
    atlas.bind_texture_unit(0);

    uint32_t num_of_triangles = 0;

    if(_debug_render_not_fill) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    for(auto const &[key, chunk] : m_chunks) {
        if(!chunk->m_chunk_vao.has_been_created()) {
            continue;
        }

        const ChunkKeyInfo key_info = ChunkKeyInfo::from_key(key);
        const int32_t z = key_info.z;
        const int32_t x = key_info.x;

        shader.upload_mat4("u_model", mat4::translate(vec3{ float(x * CHUNK_SIZE_X), 0.0f, float(z * CHUNK_SIZE_Z) }).e);

        chunk->m_chunk_vao.bind_vao();
        GL_CHECK(glDrawElements(GL_TRIANGLES, chunk->m_chunk_vao.get_ibo_count(), GL_UNSIGNED_INT, 0));

        num_of_triangles += chunk->m_chunk_vao.get_ibo_count() / 3;
    }

    DebugUI::push_text_right("triangles rendered: %d", num_of_triangles);

    if(_debug_render_not_fill) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

Chunk *World::get_chunk(const vec2i &chunk_xz, bool create_if_doesnt_exist) {
    ChunkKeyInfo key_info = ChunkKeyInfo::from_xz(chunk_xz);

    const auto hash = m_chunks.find(key_info.key);
    if(hash != m_chunks.end()) {
        return hash->second;
    }

    if(!create_if_doesnt_exist) {
        return NULL;
    }

    return this->gen_chunk(chunk_xz);
}

const Chunk *World::get_chunk(const vec2i &chunk_xz) const {
    ChunkKeyInfo key_info = ChunkKeyInfo::from_xz(chunk_xz);

    const auto hash = m_chunks.find(key_info.key);
    if(hash != m_chunks.end()) {
        return hash->second;
    } else {
        return NULL;
    }
}

Chunk *World::gen_chunk(const vec2i &chunk_xz) {
    ChunkKeyInfo key_info = ChunkKeyInfo::from_xz(chunk_xz);

    if(m_chunks.find(key_info.key) != m_chunks.end()) {
        return NULL;
    }

    return this->gen_chunk(key_info.key);
}

Block *World::get_block(const vec3i &block) {
    WorldPosition block_p = WorldPosition::from_block(block);

    Chunk *chunk = this->get_chunk(block_p.chunk);
    if(chunk == NULL) {
        /* Chunk doesn't exist */
        return NULL;
    }

    return chunk->get_block(block_p.block_rel);
}

const Block *World::get_block(const vec3i &block) const {
    WorldPosition block_p = WorldPosition::from_block(block);

    const Chunk *chunk = this->get_chunk(block_p.chunk);
    if(chunk == NULL) {
        /* Chunk doesn't exist */
        return NULL;
    }

    return chunk->get_block(block_p.block_rel);
}

const std::unordered_map<uint64_t, Chunk *> &World::get_chunk_map(void) {
    return m_chunks;
}

const size_t World::get_chunk_map_size(void) const {
    return m_chunks.size();
}

Chunk *World::gen_chunk(uint64_t key) {
    ChunkKeyInfo key_info = ChunkKeyInfo::from_key(key);

    ASSERT(m_chunks.find(key_info.key) == m_chunks.end(), "Error: Chunk already allocated!\n");

    const vec2i chunk_xz = {
        .x = key_info.x, 
        .y = key_info.z 
    };

    m_chunks[key_info.key] = new Chunk(this, chunk_xz);
    Chunk *created = m_chunks[key_info.key];

    int32_t lowest  = INT32_MAX;
    int32_t highest = INT32_MIN;

    for(int32_t x = 0; x < CHUNK_SIZE_X; ++x) {
        for(int32_t z = 0; z < CHUNK_SIZE_Z; ++z) {
            const float smooth = 0.015f;
            int32_t abs_x = x + CHUNK_SIZE_X * chunk_xz.x;
            int32_t abs_z = z + CHUNK_SIZE_Z * chunk_xz.y;
            float perlin01 = SQUARE((stb_perlin_noise3_seed(abs_x * smooth, 0.0f, abs_z * smooth, 0, 0, 0, m_world_gen_seed) + 1.0f) * 0.5f);
            // float perlin01 = (stb_perlin_noise3_seed(abs_x * smooth, 0.0f, abs_z * smooth, 0, 0, 0, m_world_gen_seed) + 1.0f) * 0.5f;
 //           float perlin01 = 0.5f;
            int32_t height = perlin01 * (CHUNK_SIZE_Y * 0.5f) + CHUNK_SIZE_Y * 0.5f;

            /*
            if(rand() % 8 == 0) {
                height += 1;
            }
            */

            if(lowest > height) lowest = height;
            if(highest < height) highest = height;

            for(int32_t y = 0; y < height; ++y) {
                Block *block = created->get_block({ x, y, z });
                if(y < height / 2) {
                    block->set_type(BlockType::COBBLESTONE);
                } else if(y >= height / 2 && y < CHUNK_SIZE_Y / 2 + 10) {
                    block->set_type(BlockType::SAND);
                } else if(y > CHUNK_SIZE_Y - 40) {
                    block->set_type(BlockType::COBBLESTONE);
                } else if(y < (height - 1)) {
                    block->set_type(BlockType::DIRT);
                } else {
                    block->set_type(BlockType::DIRT);
                    BlockInfo &info = block->get_info();
                    if(rand() % 2) {
                        info.dirt.has_grass = true;
                    }
                }
            }
        }
    }

    this->queue_chunk_vao_load(chunk_xz);

    return created;
}

vec3 real_position_from_block(const vec3i &block) {
    return {
        (float)block.x,
        (float)block.y,
        (float)block.z
    };
}

vec3i block_position_from_real(const vec3 &real) {
    return {
        (int32_t)floorf(real.x),
        (int32_t)floorf(real.y),
        (int32_t)floorf(real.z),
    };
}

vec2i chunk_position_from_block(const vec3i &block) {
    return {
        (int32_t)floorf(float(block.x) / CHUNK_SIZE_X),
        (int32_t)floorf(float(block.z) / CHUNK_SIZE_Z)
    };
}

vec3i block_relative_from_block(const vec3i &block) {
    return { 
        .x = block.x >= 0 ? block.x % CHUNK_SIZE_X : (block.x + 1) % CHUNK_SIZE_X + CHUNK_SIZE_X - 1,
        .y = block.y,
        .z = block.z >= 0 ? block.z % CHUNK_SIZE_Z : (block.z + 1) % CHUNK_SIZE_Z + CHUNK_SIZE_Z - 1,
    };
}

void calc_overlapping_blocks(vec3 pos, vec3 size, WorldPosition &min, WorldPosition &max) {
    vec3i block_min_p = block_position_from_real(pos);
    vec3i block_max_p = block_position_from_real(pos + size);
    min = WorldPosition::from_block(block_min_p);
    max = WorldPosition::from_block(block_max_p);
}

WorldPosition WorldPosition::from_real(const vec3 &real) {
    WorldPosition result;
    result.real = real;
    result.block = block_position_from_real(real);
    result.block_rel = block_relative_from_block(result.block);
    result.chunk = chunk_position_from_block(result.block);
    return result;
}

WorldPosition WorldPosition::from_block(const vec3i &block) {
    WorldPosition result;
    result.block = block;
    result.block_rel = block_relative_from_block(block);
    result.chunk = chunk_position_from_block(result.block);
    result.real = real_position_from_block(block);
    return result;
}
