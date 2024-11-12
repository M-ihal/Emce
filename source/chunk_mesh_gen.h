#pragma once

#include "chunk.h"

#include <vector>

struct ChunkMeshData {
    std::vector<ChunkVaoVertex> vertices;
    std::vector<uint32_t>       indices;
};

struct ChunkMeshGenData {
    Block *chunk_z_pos;
    Block *chunk_z_neg;
    Block *chunk_x_pos;
    Block *chunk_x_neg;
    Block *chunk_blocks;
    vec2i chunk_xz;

    ChunkMeshData chunk;
    ChunkMeshData water;
};

/* Load chunk's blocks and neighbouring chunk blocks */
void chunk_mesh_gen_data_init(ChunkMeshGenData &gen_data, World &world, vec2i chunk_xz);
void chunk_mesh_gen_data_free(ChunkMeshGenData &gen_data);

void chunk_mesh_gen(ChunkMeshGenData &gen_data);

ChunkMeshData chunk_mesh_gen_single_block(BlockType type);

