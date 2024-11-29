#pragma once

#include "chunk.h"

#include <vector>

struct ChunkMeshData {
    std::vector<ChunkVaoVertexPacked> vertices;
    std::vector<uint32_t>             indices;
};

/* @TODO : For neighbouring chunks only need neighbouring blocks... */
struct ChunkMeshGenData {
    Block chunk_z_pos[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z];
    Block chunk_z_neg[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z];
    Block chunk_x_pos[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z];
    Block chunk_x_neg[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z];
    Block chunk_x_neg_z_neg[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z];
    Block chunk_x_pos_z_pos[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z];
    Block chunk_x_neg_z_pos[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z];
    Block chunk_x_pos_z_neg[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z];
    Block chunk_blocks[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z];
    vec2i chunk_xz;

    ChunkMeshData chunk;
    ChunkMeshData water;
};

void chunk_mesh_gen_data_init(ChunkMeshGenData **gen_data_ptr, World &world, vec2i chunk_xz);
void chunk_mesh_gen_data_free(ChunkMeshGenData **gen_data_ptr);
void chunk_mesh_gen(ChunkMeshGenData *gen_data);
void chunk_mesh_gen_single_block(ChunkMeshData &mesh_data, BlockType type);
