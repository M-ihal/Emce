#pragma once

#include "common.h"
#include "shader.h"
#include "texture.h"
#include "chunk.h"

// meeh
#include <unordered_map>

// @temp
#define WORLD_CHUNK_SIZE_X 2
#define WORLD_CHUNK_SIZE_Z 2

class World {
public:
    CLASS_COPY_DISABLE(World);

    World(void);

    ~World(void);
    
    void render_chunks(const Shader &shader, const Texture &sand_texture);
    
    // @temp
    void gen_chunk_at(vec2i chunk);

private:
    // Big @todo
    std::unordered_map<uint64_t, Chunk *> m_chunks;
};
