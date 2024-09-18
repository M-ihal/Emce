#include "world.h"
#include <glew.h>

// @temp -> Pack 2 int values (x, z)[chunk coords] into one uint64_t value in order to use that value as the key in std::map
// @todo / @check

// @todo
// struct PackedChunkCoords {
//     union {
//         uint64_t packed;
//         struct {
//             int32_t chunk_x;
//             int32_t chunk_z;
//         };
//     };
// };

// @todo
static inline uint64_t pack_2x_int32(int32_t x, int32_t z) {
    const uint64_t packed = uint64_t(*(uint32_t *)&x) | (uint64_t(*(uint32_t *)&z) << 32);
    return packed;
}

// @todo
static inline void unpack_2x_int32(uint64_t packed, int32_t *x, int32_t *z) {
    *x = *(int32_t *)&packed;
    *z = *(int32_t *)((uint8_t *)&packed + 4);
}

#define PACK_2X_INT32(x, z) uint64_t(uint64_t(x) | uint64_t(z) << 32)
#define UNPACK_2X_INT32(v, x, z) { x = int32_t((v << 32) >> 32); z = int32_t(v >> 32); }

World::World(void) {
//    for(int32_t x = -4; x < 4; ++x) {
//        for(int32_t z = -4; z < 4; ++z) {
//            uint64_t packed = pack_2x_int32(x, z);
//            m_chunks[packed] = new Chunk();
//        }
//    }
}

World::~World(void) {
    for(auto const &[key, chunk] : m_chunks) {
        chunk->m_chunk_vao.delete_vao();
    }
}

void World::render_chunks(const Shader &shader, const Texture &atlas) {
    shader.use_program();
    shader.upload_int("u_texture", 0);
    atlas.bind_texture_unit(0);

    for(auto const &[key, chunk] : m_chunks) {
        int32_t x = 0;
        int32_t z = 0;
        unpack_2x_int32(key, &x, &z);
        shader.upload_mat4("u_model", mat4::translate(vec3{ float(x * CHUNK_SIZE_X), 0.0f, float(z * CHUNK_SIZE_Z) }).e);
        
        chunk->m_chunk_vao.bind_vao();
        GL_CHECK(glDrawElements(GL_TRIANGLES, chunk->m_chunk_vao.get_ibo_count(), GL_UNSIGNED_INT, 0));
    }
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

    m_chunks[packed] = new Chunk();
    return m_chunks[packed];
}

// @temp
void World::gen_chunk_at(vec2i chunk) {
    if(this->get_chunk(chunk.x, chunk.y)) {
        /* Already exists */
        return;
    }

    Chunk *const created = this->get_chunk(chunk.x, chunk.y, true);

    for(int32_t x = 0; x < CHUNK_SIZE_X; ++x) {
        for(int32_t z = 0; z < CHUNK_SIZE_Z; ++z) {
            const int32_t height = (int32_t)roundf((sinf(float(x + chunk.x * CHUNK_SIZE_X) * 0.1f * float(z + chunk.y * CHUNK_SIZE_Z) * 0.1f * 0.01f) * 0.5f + 0.5f) * (CHUNK_SIZE_Y - 8) + 8);
            for(int32_t y = 0; y < height; ++y) {
                Block &block = created->get_block(x, y, z);
                if(y < height / 2) {
                    block.set_type(BlockType::COBBLESTONE);
                } else if(y < (height - 1)) {
                    block.set_type(BlockType::DIRT);
                } else {
                    block.set_type(BlockType::SAND);
                }
            }
        }
    }

    created->update_chunk_vao();
}
