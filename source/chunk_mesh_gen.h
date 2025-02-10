#pragma once

#include "chunk.h"
#include <vector>

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
    return ChunkVaoVertexPacked {
        .packed1 = ((vx.x & 0b11111111) << 0)  | ((vx.y & 0b11111111) << 8)  | ((vx.z & 0b11111111) << 16) | ((vx.n & 0b00001111) << 24),
        .packed2 = ((vx.ts & 0b11111111) << 0) | ((vx.tc & 0b00000011) << 8) | ((vx.ao & 0b00000011) << 10)
    };
}

struct ChunkMeshData {
    std::vector<ChunkVaoVertexPacked> vertices;
    std::vector<uint32_t>             indices;
};

/* @TODO : For neighbouring chunks only need neighbouring blocks... */
struct ChunkMeshGenData {
    /* X*Y or Z*Y slice of neighboring chunks */
    BlockType chunk_z_pos[CHUNK_SIZE_X * CHUNK_SIZE_Y];
    BlockType chunk_z_neg[CHUNK_SIZE_X * CHUNK_SIZE_Y];
    BlockType chunk_x_pos[CHUNK_SIZE_Z * CHUNK_SIZE_Y];
    BlockType chunk_x_neg[CHUNK_SIZE_Z * CHUNK_SIZE_Y];

    /* Neighbouring collumn of diagonally neighboring chunks */
    BlockType chunk_x_neg_z_neg[CHUNK_SIZE_Y];
    BlockType chunk_x_pos_z_pos[CHUNK_SIZE_Y];
    BlockType chunk_x_neg_z_pos[CHUNK_SIZE_Y];
    BlockType chunk_x_pos_z_neg[CHUNK_SIZE_Y];

    /* Target chunk */
    BlockType chunk_blocks[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z];
    vec2i     chunk_coords;
 
    /* Generated vertex data */
    ChunkMeshData chunk;
    ChunkMeshData water;

    // TODO: remove
    bool has_been_dropped;
};

void chunk_mesh_gen_data_init(ChunkMeshGenData *gen_data, World &world, vec2i chunk_coords);
void chunk_mesh_gen_data_free(ChunkMeshGenData *gen_data);
void chunk_mesh_gen(ChunkMeshGenData *gen_data);
void chunk_mesh_gen_single_block(ChunkMeshData &mesh_data, BlockType block);
