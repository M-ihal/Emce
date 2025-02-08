#pragma once

#include "common.h"
#include "vertex_array.h"
#include "shader.h"
#include "texture.h"
#include "texture_array.h"
#include "blocks.h"

constexpr int32_t CHUNK_SIZE_X = 32;
constexpr int32_t CHUNK_SIZE_Y = 255;
constexpr int32_t CHUNK_SIZE_Z = 32;

static_assert(CHUNK_SIZE_X == CHUNK_SIZE_Z);

inline bool is_inside_chunk(const vec3i &block_rel) {
    return block_rel.x >= 0 && block_rel.x < CHUNK_SIZE_X
        && block_rel.y >= 0 && block_rel.y < CHUNK_SIZE_Y
        && block_rel.z >= 0 && block_rel.z < CHUNK_SIZE_Z;
}

inline uint32_t get_block_array_index(const vec3i &rel) {
    ASSERT(is_inside_chunk(rel) && "Block out of bounds!");
    return rel.x * (CHUNK_SIZE_Y * CHUNK_SIZE_Z) + rel.y * CHUNK_SIZE_Z + rel.z;
}

#define for_every_block(var_x, var_y, var_z)\
    for(int32_t var_y = 0; var_y < CHUNK_SIZE_Y; ++var_y)\
        for(int32_t var_z = 0; var_z < CHUNK_SIZE_Z; ++var_z)\
            for(int32_t var_x = 0; var_x < CHUNK_SIZE_X; ++var_x)

enum class ChunkState {
    GENERATING = 0,
    GENERATED  = 1
};

enum class ChunkMeshState {
    NOT_LOADED = 0,
    WAITING    = 1,
    QUEUED     = 2,
    LOADED     = 3,
};

class World;
class Game;

class Chunk {
public:
    CLASS_COPY_DISABLE(Chunk);

    Chunk(World *world, vec2i chunk_coords);
    ~Chunk(void);

    /* Get world containing this chunk */
    World *get_world(void);

    /* Access chunk vao */
    const VertexArray &get_vao(void);

    /* Access water vao */
    const VertexArray &get_water_vao(void);

    /* Chunk XZ values */
    vec2i get_chunk_coords(void);

    /* */
    void set_block(const vec3i &rel, BlockType type);

    /* On fail returns BlockType::_INVALID */
    BlockType get_block(const vec3i &rel);
    
    /* Copying blocks */
    void copy_blocks_into(BlockType blocks[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z]);
    void copy_blocks_from(BlockType blocks[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z]);

    /* Get current chunk state */
    ChunkState get_chunk_state(void);

    /* Set chunk state */
    void set_chunk_state(ChunkState state);

    /* Re/Sets mesh for this chunk based on gen data */
    void set_mesh(struct ChunkMeshGenData *gen_data);

    /* Get current mesh state */
    ChunkMeshState get_mesh_state(void);

    /* Set mesh state */
    void set_mesh_state(ChunkMeshState state);

    /* If should _and_ can build the mesh */
    bool should_build_mesh(void);

private:
    class World *m_owner;
    VertexArray  m_chunk_vao;
    VertexArray  m_water_vao;
    vec2i        m_chunk_coords;
    BlockType    m_blocks[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z];

    ChunkState     m_state;
    ChunkMeshState m_mesh_state;
};

