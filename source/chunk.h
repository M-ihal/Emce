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

enum class BlockType : uint8_t {
    AIR,
    SAND,
    DIRT,
    COBBLESTONE,
    TREE_LOG,
    TREE_LEAVES,
};

inline const char *block_type_string[] = {
    "Air",
    "Sand",
    "Dirt",
    "Cobblestone",
    "Tree log",
    "Tree leaves"
};

class World;

struct BlockInfo {
    BlockType type;
    union {
        struct { } air;
        struct { } sand;
        struct {
            bool has_grass;
        } dirt;
        struct { } cobblestone;
        struct { } tree_log;
        struct { } tree_leaves;
    };
};

class Block {
public:
    Block(void);

    void set_type(BlockType type);

    BlockType  get_type(void);
    BlockInfo &get_info(void);

    bool is_solid(void);
    bool is_of_type(BlockType type);

private:
    BlockInfo m_info;
};

class Chunk {
public:
    CLASS_COPY_DISABLE(Chunk);

    friend World;

    explicit Chunk(World *world, vec2i chunk_xz);
    ~Chunk(void);

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
