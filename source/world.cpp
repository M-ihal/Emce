#include "world.h"

#include "debug_ui.h"

// WorldPosition
#include "game.h"

#include <glew.h>

#define STB_PERLIN_IMPLEMENTATION
#include <stb_perlin.h>

/*
    @todo Flag chunks to regenerate vao at the end of a frame
*/

static inline uint64_t pack_2x_int32(int32_t x, int32_t z) {
    const uint64_t packed = uint64_t(*(uint32_t *)&x) | (uint64_t(*(uint32_t *)&z) << 32);
    return packed;
}

static inline void unpack_2x_int32(uint64_t packed, int32_t *x, int32_t *z) {
    *x = *(int32_t *)&packed;
    *z = *(int32_t *)((uint8_t *)&packed + 4);
}

World::World(void) {
    m_world_gen_seed = 2137;
    m_chunks.clear();
    _debug_render_not_fill = false;
}

World::~World(void) {
    this->delete_chunks();
}

void World::initialize_world(int32_t seed) {
    if(get_chunk_map_size()) {
        delete_chunks();
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

        int32_t x = 0;
        int32_t z = 0;
        unpack_2x_int32(key, &x, &z);
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
    const uint64_t packed = pack_2x_int32(chunk_xz.x, chunk_xz.y);
    const auto hash = m_chunks.find(packed);
    if(hash != m_chunks.end()) {
        return hash->second;
    }

    if(!create_if_doesnt_exist) {
        return NULL;
    }

    return this->generate_chunk(packed);
}

const Chunk *World::get_chunk(const vec2i &chunk_xz) const {
    const uint64_t packed = pack_2x_int32(chunk_xz.x, chunk_xz.y);
    const auto hash = m_chunks.find(packed);

    if(hash != m_chunks.end()) {
        return hash->second;
    } else {
        return NULL;
    }
}

Chunk *World::gen_chunk(const vec2i &chunk_xz) {
    const uint64_t packed = pack_2x_int32(chunk_xz.x, chunk_xz.y);
    if(m_chunks.find(packed) != m_chunks.end()) {
        return NULL;
    }

    return this->generate_chunk(packed);
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

Chunk *World::generate_chunk(uint64_t packed_xz) {
    int32_t chunk_x;
    int32_t chunk_z;
    unpack_2x_int32(packed_xz, &chunk_x, &chunk_z);

    m_chunks[packed_xz] = new Chunk(this, { chunk_x, chunk_z });
    Chunk *created = m_chunks[packed_xz];

    int32_t lowest = INT32_MAX;
    int32_t highest = INT32_MIN;

    for(int32_t x = 0; x < CHUNK_SIZE_X; ++x) {
        for(int32_t z = 0; z < CHUNK_SIZE_Z; ++z) {
            const float smooth = 0.01f;
            int32_t abs_x = x + CHUNK_SIZE_X * chunk_x;
            int32_t abs_z = z + CHUNK_SIZE_Z * chunk_z;
            float perlin01 = SQUARE((stb_perlin_noise3_seed(abs_x * smooth, 0.0f, abs_z * smooth, 0, 0, 0, m_world_gen_seed) + 1.0f) * 0.5f);
            const int32_t height = perlin01 * (CHUNK_SIZE_Y * 0.5f) + CHUNK_SIZE_Y * 0.5f;

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
                    block->set_type(BlockType::DIRT_WITH_GRASS);
                }
            }
        }
    }

    return created;
}
