#include "world.h"

#include <glew.h>

#define STB_PERLIN_IMPLEMENTATION
#include <stb_perlin.h>

static inline uint64_t pack_2x_int32(int32_t x, int32_t z) {
    const uint64_t packed = uint64_t(*(uint32_t *)&x) | (uint64_t(*(uint32_t *)&z) << 32);
    return packed;
}

static inline void unpack_2x_int32(uint64_t packed, int32_t *x, int32_t *z) {
    *x = *(int32_t *)&packed;
    *z = *(int32_t *)((uint8_t *)&packed + 4);
}

World::World(void) {
}

World::~World(void) {
    this->delete_chunks();
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

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    for(auto const &[key, chunk] : m_chunks) {
        int32_t x = 0;
        int32_t z = 0;
        unpack_2x_int32(key, &x, &z);
        shader.upload_mat4("u_model", mat4::translate(vec3{ float(x * CHUNK_SIZE_X), 0.0f, float(z * CHUNK_SIZE_Z) }).e);
        
        chunk->m_chunk_vao.bind_vao();
        GL_CHECK(glDrawElements(GL_TRIANGLES, chunk->m_chunk_vao.get_ibo_count(), GL_UNSIGNED_INT, 0));
    }

    // glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

Chunk *World::get_chunk(int32_t x, int32_t z, bool create_if_doesnt_exist) {
    uint64_t packed = pack_2x_int32(x, z);
    auto hash = m_chunks.find(packed);
    if(hash != m_chunks.end()) {
        return hash->second;
    }

    if(!create_if_doesnt_exist) {
        return NULL;
    }

    return this->gen_chunk(packed);
}

Chunk *World::gen_chunk(uint64_t packed_xz) {
    m_chunks[packed_xz] = new Chunk();
    Chunk *created = m_chunks[packed_xz];

    int32_t chunk_x;
    int32_t chunk_z;
    unpack_2x_int32(packed_xz, &chunk_x, &chunk_z);

    int32_t lowest = INT32_MAX;
    int32_t highest = INT32_MIN;

    int32_t seed = 2002134;
    for(int32_t x = 0; x < CHUNK_SIZE_X; ++x) {
        for(int32_t z = 0; z < CHUNK_SIZE_Z; ++z) {
            // const int32_t height = (int32_t)roundf((sinf(float(x + chunk_x * CHUNK_SIZE_X) * 0.3f * float(z + chunk_z * CHUNK_SIZE_Z) * 0.5f * 0.01f) * 0.5f + 0.5f) * (CHUNK_SIZE_Y - 8) + 8);

            const float smooth = 0.015f;
            int32_t abs_x = x + CHUNK_SIZE_X * chunk_x;
            int32_t abs_z = z + CHUNK_SIZE_Z * chunk_z;
            float perlin01 = SQUARE((stb_perlin_noise3_seed(abs_x * smooth, 0.0f, abs_z * smooth, 0, 0, 0, m_world_gen_seed) + 1.0f) * 0.5f);
 //           PRINT_FLOAT(perlin01);
            const int32_t height = perlin01 * (CHUNK_SIZE_Y * 0.5f) + CHUNK_SIZE_Y * 0.5f;

            if(lowest > height) lowest = height;
            if(highest < height) highest = height;

            for(int32_t y = 0; y < height; ++y) {
                Block &block = created->get_block(x, y, z);
                if(y < height / 2) {
                    block.set_type(BlockType::COBBLESTONE);
                } else if(y >= height / 2 && y < CHUNK_SIZE_Y / 2 + 10) {
                    block.set_type(BlockType::SAND);
                } else if(y > CHUNK_SIZE_Y - 40) {
                    block.set_type(BlockType::COBBLESTONE);
                } else if(y < (height - 1)) {
                    block.set_type(BlockType::DIRT);
                } else {
                    block.set_type(BlockType::DIRT_WITH_GRASS);
                }
            }
        }
    }

    // PRINT_INT(lowest);
    // PRINT_INT(highest);
    // fprintf(stdout, "\n");

    created->update_chunk_vao();
    return created;
}
