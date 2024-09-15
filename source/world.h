#pragma once

#include "common.h"
#include "shader.h"
#include "texture.h"
#include "chunk.h"
#include "math_types.h"

// meeh
#include <vector>
#include <tuple>

// @temp
#define WORLD_CHUNK_SIZE_X 2
#define WORLD_CHUNK_SIZE_Z 2

class World {
public:
    CLASS_COPY_DISABLE(World);

    World(void);

    ~World(void);
    
    void render_chunks(const Shader &shader, const Texture &sand_texture);

private:

    // @todo Allocate memory beforehand and then create chunks??

    // @temp
    std::vector<std::tuple<Chunk *, vec3i>> m_chunks;
};
