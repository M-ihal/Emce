#pragma once

#include "chunk.h"

#include <vector>

struct ChunkMeshGenData {
    Block x_neg_nbs[CHUNK_SIZE_Z][CHUNK_SIZE_Y];
    Block x_pos_nbs[CHUNK_SIZE_Z][CHUNK_SIZE_Y];
    Block z_neg_nbs[CHUNK_SIZE_X][CHUNK_SIZE_Y];
    Block z_pos_nbs[CHUNK_SIZE_X][CHUNK_SIZE_Y];

    Block chunk_blocks[CHUNK_SIZE_X][CHUNK_SIZE_Y][CHUNK_SIZE_Z];
    vec2i chunk_xz;

    std::vector<ChunkVaoVertex> chunk_vertices;
    std::vector<uint32_t>       chunk_indices;
    std::vector<ChunkVaoVertex> water_vertices;
    std::vector<uint32_t>       water_indices;
};

/* Load chunk's blocks and neighbouring chunk blocks */
void chunk_mesh_gen_data_init(ChunkMeshGenData &gen_data, Chunk &chunk);

void chunk_mesh_gen(ChunkMeshGenData &gen_data);
void chunk_mesh_free(ChunkMeshGenData &gen_data);
