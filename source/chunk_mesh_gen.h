#pragma once

#include "chunk.h"

#include <vector>

#define MAX_QUEUED_MESHES 16

struct ChunkMeshData {
    std::vector<ChunkVaoVertexPacked> vertices;
    std::vector<uint32_t>             indices;
};

/* @TODO : For neighbouring chunks only need neighbouring blocks... */
struct ChunkMeshGenData {
    BlockType chunk_z_pos[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z];
    BlockType chunk_z_neg[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z];
    BlockType chunk_x_pos[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z];
    BlockType chunk_x_neg[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z];
    BlockType chunk_x_neg_z_neg[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z];
    BlockType chunk_x_pos_z_pos[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z];
    BlockType chunk_x_neg_z_pos[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z];
    BlockType chunk_x_pos_z_neg[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z];
    BlockType chunk_blocks[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z];
    vec2i chunk_xz;

    ChunkMeshData chunk;
    ChunkMeshData water;

    int32_t array_index;
    bool    has_been_dropped;
};

void chunk_mesh_gen_data_init_global(void);
void chunk_mesh_gen_data_free_global(void);
bool chunk_mesh_slots_available(void);

/* If supplied_memory = true, no need to free */
void chunk_mesh_gen_data_init(ChunkMeshGenData **gen_data_ptr, World &world, vec2i chunk_xz, bool supplied_memory = false);
void chunk_mesh_gen_data_free(ChunkMeshGenData **gen_data_ptr);
void chunk_mesh_gen(ChunkMeshGenData *gen_data);
void chunk_mesh_gen_single_block(ChunkMeshData &mesh_data, BlockType type);
