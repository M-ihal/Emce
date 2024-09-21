#pragma once

#include "common.h"
#include "shader.h"
#include "texture.h"
#include "chunk.h"

// meeh
#include <unordered_map>

class World {
public:
    friend class Game; // @temp

    CLASS_COPY_DISABLE(World);

    World(void);

    ~World(void);

    /* Clears the world and sets new seed */
    // void initialize(int32_t seed);

    // @temp
    void set_seed(int32_t seed) {
        m_world_gen_seed = seed;
    }

    void delete_chunks(void);

    void render_chunks(const Shader &shader, const Texture &atlas);
    
    Chunk *get_chunk(int32_t x, int32_t z, bool create_if_doesnt_exist = false);

private:
    Chunk *gen_chunk(uint64_t packed_xz);

    int32_t m_world_gen_seed;

    // Big @todo
    std::unordered_map<uint64_t, Chunk *> m_chunks;
};
