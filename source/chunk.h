#pragma once

#include "common.h"
#include "vertex_array.h"
#include "shader.h"
#include "texture.h"

#include <vector>

inline constexpr int32_t CHUNK_SIZE_X = 16;
inline constexpr int32_t CHUNK_SIZE_Y = 256;
inline constexpr int32_t CHUNK_SIZE_Z = 16;

#define for_every_block(var_x, var_y, var_z)\
    for(int32_t var_y = 0; var_y < CHUNK_SIZE_Y; ++var_y)\
        for(int32_t var_x = 0; var_x < CHUNK_SIZE_X; ++var_x)\
            for(int32_t var_z = 0; var_z < CHUNK_SIZE_Z; ++var_z)

struct ChunkVaoVertex {
    vec3 position;
    vec3 normal;
    vec2 tex_coord;
};

struct ChunkVaoGenData {
    vec2i                       chunk;
    std::vector<ChunkVaoVertex> vertices;
    std::vector<uint32_t>       indices;
};

enum class BlockSide : uint8_t {
    Z_POS,
    Z_NEG,
    X_POS,
    X_NEG,
    Y_POS,
    Y_NEG
};

enum class BlockType : uint8_t {
    AIR = 0,
    SAND,
    DIRT,
    DIRT_WITH_GRASS,
    COBBLESTONE,
    STONE,
    TREE_LOG,
    TREE_LEAVES,
    GLASS,
    __COUNT
};

inline bool is_placable(BlockType type) {
    switch(type) {
        default: return true;

        case BlockType::AIR: {
            return false;
        };
    }
}

inline const char *block_type_string[] = {
    "Air",
    "Sand",
    "Dirt",
    "Dirt with grass",
    "Cobblestone",
    "Stone",
    "Tree log",
    "Tree leaves",
    "Glass"
};

struct Block {
    BlockType type;

    bool is_solid(void) const;
};

/* Pushes side of block vertex data to given vectors @todo simplify */
void push_block_side(std::vector<ChunkVaoVertex> &vertices, 
                     std::vector<uint32_t> &indices, 
                     BlockSide side, 
                     Block &block, 
                     int32_t x, int32_t y, int32_t z, bool centered = false);

class World;

class Chunk {
public:
    CLASS_COPY_DISABLE(Chunk);

    friend World;

    explicit Chunk(World *world, vec2i chunk_xz);
    ~Chunk(void);

    const VertexArray &get_vao(void);

    void   set_block(const vec3i &rel, BlockType type);
    Block *get_block(const vec3i &rel);
    vec2i  get_coords(void);

    static bool is_inside_chunk(const vec3i &rel);

private:
    /* Re/Generates vao for this chunk based on gen data */
    void gen_vao(const ChunkVaoGenData &gen_data);

    /* Generates data for creating VAO */
    ChunkVaoGenData gen_vao_data(void);

    class World *m_owner;
    vec2i        m_chunk_xz;
    VertexArray  m_chunk_vao;
    Block        m_blocks[CHUNK_SIZE_X][CHUNK_SIZE_Y][CHUNK_SIZE_Z];
};
