#pragma once

#include "common.h"
#include "vertex_array.h"
#include "shader.h"
#include "texture.h"

#define CHUNK_SIZE_X 16 
#define CHUNK_SIZE_Y 64
#define CHUNK_SIZE_Z 16

#define for_every_block(var_x, var_y, var_z)\
    for(int32_t var_y = 0; var_y < CHUNK_SIZE_Y; ++var_y)\
        for(int32_t var_x = 0; var_x < CHUNK_SIZE_X; ++var_x)\
            for(int32_t var_z = 0; var_z < CHUNK_SIZE_Z; ++var_z)

enum class BlockType : int32_t {
    AIR = 0, /* NONE */
    SAND
};

class Block {
public:
    Block(void);

    void set_type(BlockType type);

    bool is_of_type(BlockType type) const;

private:
    BlockType m_type;
};

class Chunk {
public:
    // temp
    friend class World;

    // CLASS_COPY_DISABLE(Chunk);
    // CLASS_MOVE_ALLOW(Chunk);

    Chunk(void);

    ~Chunk(void);

    void update_chunk_vao(void);

    Block &get_block(int32_t rel_x, int32_t rel_y, int32_t rel_z);

    const Block &get_block(int32_t rel_x, int32_t rel_y, int32_t rel_z) const;

    static bool is_inside_chunk(int32_t rel_x, int32_t rel_y, int32_t rel_z);

    //TEMP
    void render(vec3i offset, const Shader &shader, const Texture &texture); // TEMP

private:
    VertexArray m_chunk_vao;
    Block       m_blocks[CHUNK_SIZE_X][CHUNK_SIZE_Y][CHUNK_SIZE_Z];
};