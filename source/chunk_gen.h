#pragma once

#include "chunk.h"

struct WorldGenSeed { uint32_t seed; };

struct ChunkGenData {
    vec2i chunk_xz;
    WorldGenSeed seed;
    Block blocks[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z];
};

/* Initializes the structure */
void chunk_gen_data_init(ChunkGenData &gen, vec2i chunk_xz, WorldGenSeed seed);

/* Fills the block array with chunk data */
void chunk_gen(ChunkGenData &gen);

/* Fill 2D array with chunk height values */
void chunk_gen_height_map(ChunkGenData &gen, int32_t height_map[CHUNK_SIZE_X][CHUNK_SIZE_Z]);
