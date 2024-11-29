#pragma once

#include "common.h"
#include "vertex_array.h"
#include "shader.h"
#include "texture.h"
#include "texture_array.h"

constexpr int32_t CHUNK_SIZE_X = 32;
constexpr int32_t CHUNK_SIZE_Y = 255;
constexpr int32_t CHUNK_SIZE_Z = 32;

#define for_every_block(var_x, var_y, var_z)\
    for(int32_t var_y = 0; var_y < CHUNK_SIZE_Y; ++var_y)\
        for(int32_t var_x = 0; var_x < CHUNK_SIZE_X; ++var_x)\
            for(int32_t var_z = 0; var_z < CHUNK_SIZE_Z; ++var_z)

struct ChunkMeshGenData;

struct ChunkVaoVertex {
    uint32_t x;     
    uint32_t y;
    uint32_t z;
    uint32_t n;
    uint32_t tc;
    uint32_t ts;
    uint32_t ao;
};

struct ChunkVaoVertexPacked {
    uint32_t packed1; /* 8:x, 8:y, 8:z, 4:normal - 4:free */
    uint32_t packed2; /* 8:tex_slot, 2:texcoord, 2:ambient_occlusion - 20:free */
};

inline ChunkVaoVertexPacked pack_chunk_vertex(ChunkVaoVertex vx) {
    ChunkVaoVertexPacked v = { };
    v.packed1 = ((vx.x & 0b11111111) << 0)
              | ((vx.y & 0b11111111) << 8) 
              | ((vx.z & 0b11111111) << 16) 
              | ((vx.n & 0b00001111) << 24);

    v.packed2 = ((vx.ts & 0b11111111) << 0)
              | ((vx.tc & 0b00000011) << 8)
              | ((vx.ao & 0b00000011) << 10);
    return v;
}

#define BLOCK_SIDE_COUNT 6
enum class BlockSide : uint8_t {
    X_NEG,
    X_POS,
    Z_NEG,
    Z_POS,
    Y_NEG,
    Y_POS,
};

inline constexpr vec3i get_side_normal(BlockSide side) {
    switch(side) {
        case BlockSide::X_NEG: return vec3i{ -1,  0,  0 };
        case BlockSide::X_POS: return vec3i{ +1,  0,  0 };
        case BlockSide::Z_NEG: return vec3i{  0,  0, -1 };
        case BlockSide::Z_POS: return vec3i{  0,  0,  1 };
        case BlockSide::Y_NEG: return vec3i{  0, -1,  0 };
        case BlockSide::Y_POS: return vec3i{  0, +1,  0 };
    }
    INVALID_CODE_PATH;
    return { };
}

inline constexpr BlockSide get_side_from_normal(const vec3i &normal) {
    if(normal == vec3i{ -1, 0, 0 }) {
        return BlockSide::X_NEG;
    } else if(normal == vec3i{ 1, 0, 0 }) {
        return BlockSide::X_POS;
    } if(normal == vec3i{ 0, 0, -1 }) {
        return BlockSide::Z_NEG;
    } else if(normal == vec3i{ 0, 0, 1 }) {
        return BlockSide::Z_POS;
    } else if(normal == vec3i{ 0, -1, 0 }) {
        return BlockSide::Y_NEG;
    } else if(normal == vec3i{ 0, 1, 0 }) {
        return BlockSide::Y_POS;
    }

    INVALID_CODE_PATH;
    return (BlockSide)0;
}

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
    CACTUS,
    _COUNT,
    _INVALID
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
    "Water",
    "Cactus"
};

static_assert(ARRAY_COUNT(block_type_string) == (int32_t)BlockType::_COUNT);

enum BlockTypeFlags : uint32_t {
    IS_PLACABLE      = 1 << 0,
    IS_SOLID         = 1 << 1,
    IS_NOT_VISIBLE   = 1 << 2,
    IS_NOT_TARGETABLE = 1 << 4,
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
        case BlockType::CACTUS: return IS_PLACABLE | IS_SOLID;
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
    TEX_CACTUS_SIDE,
    TEX_CACTUS_TOP,
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
    { 3, 0 },
    { 3, 1 },
};

/* Upload block pixels from atlas to _created_ TextureArray */
void init_block_texture_array(TextureArray &texture_array, const char *atlas_filepath);

/* Upload water pixels from image to TextureArray */
void init_water_texture_array(TextureArray &texture_array, const char *water_filepath);

enum class ChunkState {
    GENERATING,
    GENERATED
};

enum class ChunkMeshState {
    NOT_LOADED,
    WAITING,
    QUEUED,   
    LOADED,
};

class Chunk {
public:
    CLASS_COPY_DISABLE(Chunk);

    friend class World;

    explicit Chunk(World *world, vec2i chunk_xz);
    ~Chunk(void);

    /* Get world containing this chunk */
    World *get_world(void);

    /* Access chunk vao */
    const VertexArray &get_vao(void);

    /* Access water vao */
    const VertexArray &get_water_vao(void);

    /* Delete all vaos */
    void delete_vaos(void);

    /* Sets relative block */
    void set_block(const vec3i &rel, BlockType type);

    /* Gets relative block */
    BlockType get_block(const vec3i &rel);

    /* Gets neighbouring block to rel block */
    BlockType get_block_neighbour(const vec3i &rel, BlockSide side, bool check_other_chunk = false);

    /* Get pointer to block data */
    BlockType *get_block_data(void);

    /* Get chunk xz position */
    vec2i get_chunk_xz(void);
    
    /* Sets ChunkMeshState to WAITING so next frame it will get checked for mesh reload */
    void set_wait_for_mesh_reload(void);

    /* Sets chunk blocks */
    void set_chunk_blocks(BlockType blocks[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z]);

    /* Get current chunk state */
    ChunkState get_chunk_state(void);

    /* Set chunk state */
    void set_chunk_state(ChunkState state);

    /* Re/Sets mesh for this chunk based on gen data */
    void set_mesh(ChunkMeshGenData *gen_data);

    /* Get current mesh state */
    ChunkMeshState get_mesh_state(void);

    /* Set mesh state */
    void set_mesh_state(ChunkMeshState state);

    /* Make chunk do appear animation when mesh gets created */
    void set_appear_animation(void);

    /* Calc chunk y position offset for rendering */
    float get_chunk_render_offset_y(void);

private:
    class World *m_owner;
    vec2i        m_chunk_xz;
    VertexArray  m_chunk_vao;
    VertexArray  m_water_vao;
    BlockType    m_blocks[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z];

    ChunkState m_state;
    ChunkMeshState m_mesh_state;

    bool  m_appear_do_anim = false;
    float m_appear_timer = 1.0f;

    bool  m_should_unload;
    float m_unload_timer;
};

inline bool is_inside_chunk(const vec3i &block_rel) {
    return block_rel.x >= 0 && block_rel.x < CHUNK_SIZE_X
        && block_rel.y >= 0 && block_rel.y < CHUNK_SIZE_Y
        && block_rel.z >= 0 && block_rel.z < CHUNK_SIZE_Z;
}

/* Ignores Y edges... */
inline bool is_block_on_chunk_edge(const vec3i block_rel, BlockSide side) {
    switch(side) {
        default: {
            INVALID_CODE_PATH;
            return false;
        }
        case BlockSide::X_NEG: {
            return block_rel.x == 0;
        }
        case BlockSide::X_POS: {
            return block_rel.x == CHUNK_SIZE_X - 1;
        }
        case BlockSide::Z_NEG: {
            return block_rel.z == 0;
        }
        case BlockSide::Z_POS: {
            return block_rel.z == CHUNK_SIZE_Z - 1;
        }
        case BlockSide::Y_NEG: {
            return block_rel.y == 0;
        }
        case BlockSide::Y_POS: {
            return block_rel.y == CHUNK_SIZE_Y - 1;
        }
    }
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
