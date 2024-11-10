#pragma once

#include "common.h"
#include "vertex_array.h"
#include "shader.h"
#include "texture.h"
#include "texture_array.h"

#include <meh_dynamic_array.h>

#include <vector>

inline constexpr int32_t CHUNK_SIZE_X = 32;
inline constexpr int32_t CHUNK_SIZE_Y = 256;
inline constexpr int32_t CHUNK_SIZE_Z = 32;

#define for_every_block(var_x, var_y, var_z)\
    for(int32_t var_y = 0; var_y < CHUNK_SIZE_Y; ++var_y)\
        for(int32_t var_x = 0; var_x < CHUNK_SIZE_X; ++var_x)\
            for(int32_t var_z = 0; var_z < CHUNK_SIZE_Z; ++var_z)

struct ChunkVaoVertex {
    vec3 position;
    vec3 normal;
    vec2 tex_coord;
    float tex_slot;
};

struct ChunkVaoGenData {
    vec2i chunk;

    std::vector<ChunkVaoVertex> chunk_vertices;
    std::vector<uint32_t>       chunk_indices;
    std::vector<ChunkVaoVertex> water_vertices;
    std::vector<uint32_t>       water_indices;
};

void init_chunk_vao_gen_data(ChunkVaoGenData &gen_data);
void free_chunk_vao_gen_data(ChunkVaoGenData &gen_data);

#define BLOCK_SIDE_COUNT 6
enum class BlockSide : uint8_t {
    Z_POS,
    Z_NEG,
    X_POS,
    X_NEG,
    Y_POS,
    Y_NEG,
};

inline constexpr vec3i get_block_side_dir(BlockSide side) {
    switch(side) {
        case BlockSide::Z_POS: return vec3i{  0,  0,  1 };
        case BlockSide::Z_NEG: return vec3i{  0,  0, -1 };
        case BlockSide::X_POS: return vec3i{ +1,  0,  0 };
        case BlockSide::X_NEG: return vec3i{ -1,  0,  0 };
        case BlockSide::Y_POS: return vec3i{  0, +1,  0 };
        case BlockSide::Y_NEG: return vec3i{  0, -1,  0 };
    }
    INVALID_CODE_PATH;
    return { };
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
    GRASS,
    WATER,
    _COUNT
};

inline const char *block_type_string[] = {
    "Air",
    "Sand",
    "Dirt",
    "Dirt with grass",
    "Cobblestone",
    "Stone",
    "Tree log",
    "Tree leaves",
    "Glass",
    "Grass",
    "Water"
};

static_assert(ARRAY_COUNT(block_type_string) == (int32_t)BlockType::_COUNT);

enum BlockTypeFlags : uint32_t {
    IS_PLACABLE      = 1 << 0,
    IS_SOLID         = 1 << 1,
    IS_NOT_VISIBLE   = 1 << 2,
    HAS_TRANSPARENCY = 1 << 16,
};

inline uint32_t get_block_flags(BlockType type) {
    switch(type) {

        default: {
            return IS_NOT_VISIBLE;
        }

        case BlockType::SAND: return IS_SOLID | IS_PLACABLE;
        case BlockType::DIRT: return IS_SOLID | IS_PLACABLE;
        case BlockType::DIRT_WITH_GRASS: return IS_SOLID | IS_PLACABLE;
        case BlockType::COBBLESTONE: return IS_SOLID | IS_PLACABLE;
        case BlockType::STONE: return IS_SOLID | IS_PLACABLE;
        case BlockType::TREE_LOG: return IS_SOLID | IS_PLACABLE;
        case BlockType::TREE_LEAVES: return IS_SOLID | IS_PLACABLE | HAS_TRANSPARENCY;
        case BlockType::GLASS: return IS_SOLID | IS_PLACABLE | HAS_TRANSPARENCY;
        case BlockType::GRASS: return IS_PLACABLE | HAS_TRANSPARENCY;

        case BlockType::WATER: return IS_PLACABLE | HAS_TRANSPARENCY;
    }
}

enum BlockTextureID : uint32_t {
    TEX_SAND = 0,
    TEX_DIRT,
    TEX_DIRT_WITH_GRASS_SIDE,
    TEX_DIRT_WITH_GRASS_TOP,
    TEX_COBBLESTONE,
    TEX_STONE,
    TEX_TREE_LOG_SIDE,
    TEX_TREE_LOG_TOP,
    TEX_TREE_LEAVES,
    TEX_GLASS,
    TEX_GRASS,
    TEX_WATER,
    _TEX_COUNT
};

inline vec2i block_texture_atlas_positions[BlockTextureID::_TEX_COUNT] = {
    { 0, 0 },
    { 1, 0 },
    { 1, 1 },
    { 1, 2 },
    { 2, 0 },
    { 2, 1 },
    { 7, 1 },
    { 7, 0 },
    { 7, 2 },
    { 0, 1 },
    { 1, 3 },
    { 31, 0 },
};

/* Upload block pixels from atlas to _created_ TextureArray */
void init_block_texture_array(TextureArray &texture_array, const char *atlas_filepath);

/* Upload water pixels from image to TextureArray */
void init_water_texture_array(TextureArray &texture_array, const char *water_filepath);

/* Generate vertices for single block vao, centered */
void gen_single_block_vao_data(BlockType type, std::vector<ChunkVaoVertex> &vertices, std::vector<uint32_t> &indices);

struct Block {
    BlockType type;
};

class Chunk {
public:
    CLASS_COPY_DISABLE(Chunk);

    friend class World;

    explicit Chunk(World *world, vec2i chunk_xz);
    ~Chunk(void);

    /* Access chunk vao */
    const VertexArray &get_vao(void);

    /* Access water vao */
    const VertexArray &get_water_vao(void);

    /* Sets relative block */
    void set_block(const vec3i &rel, BlockType type);

    /* Gets relative block */
    Block *get_block(const vec3i &rel);

    /* Gets neighbouring block to rel block */
    Block *get_block_neighbour(const vec3i &rel, BlockSide side, bool check_other_chunk = false);

    /* Get chunk xz position */
    vec2i get_coords(void);
    
private:
    /* Re/Generates vao for this chunk based on gen data */
    void gen_vao(ChunkVaoGenData &gen_data);

    /* Generates data for creating VAO */
    ChunkVaoGenData gen_vao_data(void);

    class World *m_owner;
    vec2i        m_chunk_xz;
    VertexArray  m_chunk_vao;
    VertexArray  m_water_vao;
    Block        m_blocks[CHUNK_SIZE_X][CHUNK_SIZE_Y][CHUNK_SIZE_Z];

    bool  m_should_unload;
    float m_unload_timer;
};

inline bool is_inside_chunk(const vec3i &block_rel) {
    return block_rel.x >= 0 && block_rel.x < CHUNK_SIZE_X
        && block_rel.y >= 0 && block_rel.y < CHUNK_SIZE_Y
        && block_rel.z >= 0 && block_rel.z < CHUNK_SIZE_Z;
}

inline bool is_block_on_x_neg_edge(const vec3i &block_rel) {
    return block_rel.x == 0;
}

inline bool is_block_on_x_pos_edge(const vec3i &block_rel) {
    return block_rel.x == CHUNK_SIZE_X - 1;
}

inline bool is_block_on_z_neg_edge(const vec3i &block_rel) {
    return block_rel.z == 0;
}

inline bool is_block_on_z_pos_edge(const vec3i &block_rel) {
    return block_rel.z == CHUNK_SIZE_Z - 1;
}
