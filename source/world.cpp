#include "world.h"

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
    for(int32_t x = -4; x < 4; ++x) {
        for(int32_t z = -4; z < 4; ++z) {
            uint64_t packed = pack_2x_int32(x, z);
            m_chunks[packed] = new Chunk();
        }
    }
}

World::~World(void) {
    for(auto const &[key, chunk] : m_chunks) {
        chunk->m_chunk_vao.delete_vao();
    }
}

void World::render_chunks(const Shader &shader, const Texture &sand_texture) {
    for(auto const &[key, chunk] : m_chunks) {
        int32_t x = 0;
        int32_t z = 0;
        unpack_2x_int32(key, &x, &z);
        chunk->render({ x, 0, z }, shader, sand_texture);
    }
}

void World::gen_chunk_at(vec2i chunk) {
    uint64_t packed = pack_2x_int32(chunk.x, chunk.y);
    if(m_chunks.find(packed) == m_chunks.end()) {
        m_chunks[packed] = new Chunk();
    }
}
