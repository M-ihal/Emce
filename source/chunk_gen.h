#pragma once

#include "chunk.h"

struct WorldGenSeed {
    uint32_t seed;
};

enum : uint8_t {
    BIOME_PLAINS = 0,
    BIOME_DESERT = 1,
    BIOME_FOREST = 2,
    BIOME_STONE  = 3,
    BIOME__COUNT
};

struct ChunkGenData {
    vec2i chunk_xz;
    WorldGenSeed seed;
    BlockType blocks[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z];
};

void chunk_gen_data_init(ChunkGenData &gen, vec2i chunk_xz, WorldGenSeed seed);
void chunk_gen(ChunkGenData &gen);
void chunk_gen_height_map(ChunkGenData &gen, int32_t height_map[CHUNK_SIZE_X][CHUNK_SIZE_Z]);
void chunk_gen_biome_map(ChunkGenData &gen, int32_t biome_map[CHUNK_SIZE_X][CHUNK_SIZE_Z]);

struct BlockOffset {
    BlockType type;
    vec3i offset;
};

inline BlockOffset cactus_blocks[] = {
    { BlockType::CACTUS, { 0, 0, 0 } },
    { BlockType::CACTUS, { 0, 1, 0 } },
    { BlockType::CACTUS, { 0, 2, 0 } },
};

inline BlockOffset oak_tree_blocks[] = {
    { BlockType::TREE_OAK_LOG, {0, 0, 0} },
    { BlockType::TREE_OAK_LOG, {0, 1, 0} },
    { BlockType::TREE_OAK_LOG, {0, 2, 0} },
    { BlockType::TREE_OAK_LOG, {0, 3, 0} },
    { BlockType::TREE_OAK_LOG, {0, 4, 0} },
    { BlockType::TREE_OAK_LOG, {0, 5, 0} },
    { BlockType::TREE_OAK_LEAVES, { 0, 6, 0 } },
    { BlockType::TREE_OAK_LEAVES, {-1, 6, 0 } },
    { BlockType::TREE_OAK_LEAVES, { 1, 6, 0 } },
    { BlockType::TREE_OAK_LEAVES, { 0, 6, -1} },
    { BlockType::TREE_OAK_LEAVES, { 0, 6, +1} },
    { BlockType::TREE_OAK_LEAVES, {-1, 5, 0 } },
    { BlockType::TREE_OAK_LEAVES, { 1, 5, 0 } },
    { BlockType::TREE_OAK_LEAVES, { 0, 5, -1} },
    { BlockType::TREE_OAK_LEAVES, { 0, 5, +1} },
    { BlockType::TREE_OAK_LEAVES, { +1, 5, +1} },
    { BlockType::TREE_OAK_LEAVES, { -1, 5, -1} },
    { BlockType::TREE_OAK_LEAVES, {-1, 4, 0 } },
    { BlockType::TREE_OAK_LEAVES, { 1, 4, 0 } },
    { BlockType::TREE_OAK_LEAVES, { 0, 4, -1} },
    { BlockType::TREE_OAK_LEAVES, { 0, 4, +1} },
    { BlockType::TREE_OAK_LEAVES, {-1, 4, -1 } },
    { BlockType::TREE_OAK_LEAVES, { 1, 4, 1 } },
    { BlockType::TREE_OAK_LEAVES, {-1, 4, 1} },
    { BlockType::TREE_OAK_LEAVES, { 1, 4, -1} },
    { BlockType::TREE_OAK_LEAVES, {-1, 3, 0 } },
    { BlockType::TREE_OAK_LEAVES, { 1, 3, 0 } },
    { BlockType::TREE_OAK_LEAVES, { 0, 3, -1} },
    { BlockType::TREE_OAK_LEAVES, { 0, 3, +1} },
    { BlockType::TREE_OAK_LEAVES, {-1, 3, -1 } },
    { BlockType::TREE_OAK_LEAVES, { 1, 3, 1 } },
    { BlockType::TREE_OAK_LEAVES, {-1, 3, 1} },
    { BlockType::TREE_OAK_LEAVES, { 1, 3, -1} },
    { BlockType::TREE_OAK_LEAVES, {-2, 4, 0 } },
    { BlockType::TREE_OAK_LEAVES, {-2, 4, 1 } },
    { BlockType::TREE_OAK_LEAVES, {-2, 4, -1 } },
    { BlockType::TREE_OAK_LEAVES, { 2, 4, 0 } },
    { BlockType::TREE_OAK_LEAVES, { 2, 4, 1 } },
    { BlockType::TREE_OAK_LEAVES, { 2, 4, -1 } },
    { BlockType::TREE_OAK_LEAVES, { 0, 4, -2} },
    { BlockType::TREE_OAK_LEAVES, { -1, 4, -2} },
    { BlockType::TREE_OAK_LEAVES, { 1, 4, -2} },
    { BlockType::TREE_OAK_LEAVES, { 0, 4, 2} },
    { BlockType::TREE_OAK_LEAVES, { -1, 4, 2} },
    { BlockType::TREE_OAK_LEAVES, { 1, 4, 2} },
    { BlockType::TREE_OAK_LEAVES, {-2, 3, 0 } },
    { BlockType::TREE_OAK_LEAVES, {-2, 3, 1 } },
    { BlockType::TREE_OAK_LEAVES, {-2, 3, -1 } },
    { BlockType::TREE_OAK_LEAVES, { 2, 3, 0 } },
    { BlockType::TREE_OAK_LEAVES, { 2, 3, 1 } },
    { BlockType::TREE_OAK_LEAVES, { 2, 3, -1 } },
    { BlockType::TREE_OAK_LEAVES, { 0, 3, -2} },
    { BlockType::TREE_OAK_LEAVES, { -1, 3, -2} },
    { BlockType::TREE_OAK_LEAVES, { 1, 3, -2} },
    { BlockType::TREE_OAK_LEAVES, { 0, 3, 2} },
    { BlockType::TREE_OAK_LEAVES, { -1, 3, 2} },
    { BlockType::TREE_OAK_LEAVES, { 1, 3, 2} },
};

inline BlockOffset birch_tree_blocks[] = {
    { BlockType::TREE_BIRCH_LOG, {0, 0, 0} },
    { BlockType::TREE_BIRCH_LOG, {0, 1, 0} },
    { BlockType::TREE_BIRCH_LOG, {0, 2, 0} },
    { BlockType::TREE_BIRCH_LOG, {0, 3, 0} },
    { BlockType::TREE_BIRCH_LOG, {0, 4, 0} },
    { BlockType::TREE_BIRCH_LOG, {0, 5, 0} },
    { BlockType::TREE_BIRCH_LEAVES, { 0, 6, 0 } },
    { BlockType::TREE_BIRCH_LEAVES, {-1, 6, 0 } },
    { BlockType::TREE_BIRCH_LEAVES, { 1, 6, 0 } },
    { BlockType::TREE_BIRCH_LEAVES, { 0, 6, -1} },
    { BlockType::TREE_BIRCH_LEAVES, { 0, 6, +1} },
    { BlockType::TREE_BIRCH_LEAVES, {-1, 5, 0 } },
    { BlockType::TREE_BIRCH_LEAVES, { 1, 5, 0 } },
    { BlockType::TREE_BIRCH_LEAVES, { 0, 5, -1} },
    { BlockType::TREE_BIRCH_LEAVES, { 0, 5, +1} },
    { BlockType::TREE_BIRCH_LEAVES, { +1, 5, +1} },
    { BlockType::TREE_BIRCH_LEAVES, { -1, 5, -1} },
    { BlockType::TREE_BIRCH_LEAVES, {-1, 4, 0 } },
    { BlockType::TREE_BIRCH_LEAVES, { 1, 4, 0 } },
    { BlockType::TREE_BIRCH_LEAVES, { 0, 4, -1} },
    { BlockType::TREE_BIRCH_LEAVES, { 0, 4, +1} },
    { BlockType::TREE_BIRCH_LEAVES, {-1, 4, -1 } },
    { BlockType::TREE_BIRCH_LEAVES, { 1, 4, 1 } },
    { BlockType::TREE_BIRCH_LEAVES, {-1, 4, 1} },
    { BlockType::TREE_BIRCH_LEAVES, { 1, 4, -1} },
    { BlockType::TREE_BIRCH_LEAVES, {-1, 3, 0 } },
    { BlockType::TREE_BIRCH_LEAVES, { 1, 3, 0 } },
    { BlockType::TREE_BIRCH_LEAVES, { 0, 3, -1} },
    { BlockType::TREE_BIRCH_LEAVES, { 0, 3, +1} },
    { BlockType::TREE_BIRCH_LEAVES, {-1, 3, -1 } },
    { BlockType::TREE_BIRCH_LEAVES, { 1, 3, 1 } },
    { BlockType::TREE_BIRCH_LEAVES, {-1, 3, 1} },
    { BlockType::TREE_BIRCH_LEAVES, { 1, 3, -1} },
    { BlockType::TREE_BIRCH_LEAVES, {-2, 4, 0 } },
    { BlockType::TREE_BIRCH_LEAVES, {-2, 4, 1 } },
    { BlockType::TREE_BIRCH_LEAVES, {-2, 4, -1 } },
    { BlockType::TREE_BIRCH_LEAVES, { 2, 4, 0 } },
    { BlockType::TREE_BIRCH_LEAVES, { 2, 4, 1 } },
    { BlockType::TREE_BIRCH_LEAVES, { 2, 4, -1 } },
    { BlockType::TREE_BIRCH_LEAVES, { 0, 4, -2} },
    { BlockType::TREE_BIRCH_LEAVES, { -1, 4, -2} },
    { BlockType::TREE_BIRCH_LEAVES, { 1, 4, -2} },
    { BlockType::TREE_BIRCH_LEAVES, { 0, 4, 2} },
    { BlockType::TREE_BIRCH_LEAVES, { -1, 4, 2} },
    { BlockType::TREE_BIRCH_LEAVES, { 1, 4, 2} },
    { BlockType::TREE_BIRCH_LEAVES, {-2, 3, 0 } },
    { BlockType::TREE_BIRCH_LEAVES, {-2, 3, 1 } },
    { BlockType::TREE_BIRCH_LEAVES, {-2, 3, -1 } },
    { BlockType::TREE_BIRCH_LEAVES, { 2, 3, 0 } },
    { BlockType::TREE_BIRCH_LEAVES, { 2, 3, 1 } },
    { BlockType::TREE_BIRCH_LEAVES, { 2, 3, -1 } },
    { BlockType::TREE_BIRCH_LEAVES, { 0, 3, -2} },
    { BlockType::TREE_BIRCH_LEAVES, { -1, 3, -2} },
    { BlockType::TREE_BIRCH_LEAVES, { 1, 3, -2} },
    { BlockType::TREE_BIRCH_LEAVES, { 0, 3, 2} },
    { BlockType::TREE_BIRCH_LEAVES, { -1, 3, 2} },
    { BlockType::TREE_BIRCH_LEAVES, { 1, 3, 2} },
};

