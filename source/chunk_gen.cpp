#include "chunk_gen.h"
#include <PerlinNoise.hpp>

struct BlockOffset {
    BlockType type;
    vec3i offset;
};

BlockOffset oak_tree_blocks[] = {
    // Trunk
    { BlockType::TREE_LOG, {0, 0, 0} },
    { BlockType::TREE_LOG, {0, 1, 0} },
    { BlockType::TREE_LOG, {0, 2, 0} },
    { BlockType::TREE_LOG, {0, 3, 0} },
    { BlockType::TREE_LOG, {0, 4, 0} },
    { BlockType::TREE_LOG, {0, 5, 0} },
    
    { BlockType::TREE_LEAVES, { 0, 6, 0 } },
    { BlockType::TREE_LEAVES, {-1, 6, 0 } },
    { BlockType::TREE_LEAVES, { 1, 6, 0 } },
    { BlockType::TREE_LEAVES, { 0, 6, -1} },
    { BlockType::TREE_LEAVES, { 0, 6, +1} },

    { BlockType::TREE_LEAVES, {-1, 5, 0 } },
    { BlockType::TREE_LEAVES, { 1, 5, 0 } },
    { BlockType::TREE_LEAVES, { 0, 5, -1} },
    { BlockType::TREE_LEAVES, { 0, 5, +1} },

    { BlockType::TREE_LEAVES, { +1, 5, +1} },
    { BlockType::TREE_LEAVES, { -1, 5, -1} },

    { BlockType::TREE_LEAVES, {-1, 4, 0 } },
    { BlockType::TREE_LEAVES, { 1, 4, 0 } },
    { BlockType::TREE_LEAVES, { 0, 4, -1} },
    { BlockType::TREE_LEAVES, { 0, 4, +1} },
    { BlockType::TREE_LEAVES, {-1, 4, -1 } },
    { BlockType::TREE_LEAVES, { 1, 4, 1 } },
    { BlockType::TREE_LEAVES, {-1, 4, 1} },
    { BlockType::TREE_LEAVES, { 1, 4, -1} },

    { BlockType::TREE_LEAVES, {-1, 3, 0 } },
    { BlockType::TREE_LEAVES, { 1, 3, 0 } },
    { BlockType::TREE_LEAVES, { 0, 3, -1} },
    { BlockType::TREE_LEAVES, { 0, 3, +1} },
    { BlockType::TREE_LEAVES, {-1, 3, -1 } },
    { BlockType::TREE_LEAVES, { 1, 3, 1 } },
    { BlockType::TREE_LEAVES, {-1, 3, 1} },
    { BlockType::TREE_LEAVES, { 1, 3, -1} },


    { BlockType::TREE_LEAVES, {-2, 4, 0 } },
    { BlockType::TREE_LEAVES, {-2, 4, 1 } },
    { BlockType::TREE_LEAVES, {-2, 4, -1 } },

    { BlockType::TREE_LEAVES, { 2, 4, 0 } },
    { BlockType::TREE_LEAVES, { 2, 4, 1 } },
    { BlockType::TREE_LEAVES, { 2, 4, -1 } },

    { BlockType::TREE_LEAVES, { 0, 4, -2} },
    { BlockType::TREE_LEAVES, { -1, 4, -2} },
    { BlockType::TREE_LEAVES, { 1, 4, -2} },

    { BlockType::TREE_LEAVES, { 0, 4, 2} },
    { BlockType::TREE_LEAVES, { -1, 4, 2} },
    { BlockType::TREE_LEAVES, { 1, 4, 2} },


    { BlockType::TREE_LEAVES, {-2, 3, 0 } },
    { BlockType::TREE_LEAVES, {-2, 3, 1 } },
    { BlockType::TREE_LEAVES, {-2, 3, -1 } },

    { BlockType::TREE_LEAVES, { 2, 3, 0 } },
    { BlockType::TREE_LEAVES, { 2, 3, 1 } },
    { BlockType::TREE_LEAVES, { 2, 3, -1 } },

    { BlockType::TREE_LEAVES, { 0, 3, -2} },
    { BlockType::TREE_LEAVES, { -1, 3, -2} },
    { BlockType::TREE_LEAVES, { 1, 3, -2} },

    { BlockType::TREE_LEAVES, { 0, 3, 2} },
    { BlockType::TREE_LEAVES, { -1, 3, 2} },
    { BlockType::TREE_LEAVES, { 1, 3, 2} },
};

static inline uint32_t get_block_array_index(const vec3i &block_rel) {
    return block_rel.x * (CHUNK_SIZE_Y * CHUNK_SIZE_Z) + block_rel.y * CHUNK_SIZE_Z + block_rel.z;
}

static inline Block get_block(ChunkGenData &gen, const vec3i &block_rel) {
    return gen.blocks[get_block_array_index(block_rel)];
}

static inline void set_block(ChunkGenData &gen, const vec3i &block_rel, BlockType type) {
    gen.blocks[get_block_array_index(block_rel)].type = type;
}

void chunk_gen_data_init(ChunkGenData &gen, vec2i chunk_xz, WorldGenSeed seed) {
    gen.chunk_xz = chunk_xz;
    gen.seed = seed;
}

void chunk_gen(ChunkGenData &gen) {
    /* Clear chunk to 0 (AIR) */
    memset(gen.blocks, (uint8_t)BlockType::AIR, CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * sizeof(Block));

    int32_t height_map[CHUNK_SIZE_X][CHUNK_SIZE_Z];
    chunk_gen_height_map(gen, height_map);

    int32_t ocean_level = 64;

    for(int32_t x = 0; x < CHUNK_SIZE_X; ++x) {
        for(int32_t z = 0; z < CHUNK_SIZE_Z; ++z) {

            /* Fill structure of the chunk */
            for(int32_t y = 0; y < height_map[x][z]; ++y) {
                set_block(gen, vec3i{ x, y, z }, BlockType::STONE);
            }

            /* Fill Ground section */
            const int32_t ground_height = 3;
            for(int32_t y = height_map[x][z] - 1; y >= height_map[x][z] - 1 - ground_height; y -= 1) {
                if(y <= ocean_level ) {
                    set_block(gen, vec3i{ x, y, z }, BlockType::SAND);
                } else {
                    if(y == height_map[x][z] - 1) {
                        set_block(gen, vec3i{ x, y, z }, BlockType::DIRT_WITH_GRASS);
                    } else {
                        set_block(gen, vec3i{ x, y, z }, BlockType::DIRT);
                    }
                }
            }

            // @TEMP STUFF

            if(height_map[x][z] > ocean_level) {
                // PLANT GRASS
                if(rand() % 40 == 0) {
                    if(get_block(gen, vec3i{ x, height_map[x][z], z }).type == BlockType::AIR) {
                        set_block(gen, vec3i{ x, height_map[x][z], z }, BlockType::GRASS);
                    }
                }

                // TREES
                if(rand() % 585 == 0) {
                    if(x >= 2 && x < CHUNK_SIZE_X - 2 && z >= 2 && z < CHUNK_SIZE_Z - 2 && (height_map[x][z] + 6) < CHUNK_SIZE_Y) {
                        for(int32_t index = 0; index < ARRAY_COUNT(oak_tree_blocks); ++index) {
                            BlockOffset block = oak_tree_blocks[index];

                            vec3i block_rel = vec3i{ x, height_map[x][z], z } + block.offset;
                            if(get_block(gen, block_rel).type == BlockType::AIR) {
                                set_block(gen, block_rel, block.type);
                            }
                        }
                    }
                }
            } else {
                int32_t y = ocean_level;
                while(y-- > height_map[x][z]) {
                    set_block(gen, vec3i{ x, y, z }, BlockType::WATER);
                }
            }
        }
    }
}

void chunk_gen_height_map(ChunkGenData &gen, int32_t height_map[CHUNK_SIZE_X][CHUNK_SIZE_Z]) {
    siv::PerlinNoise perlin(gen.seed.seed);

    const int32_t octaves = 6;
    const double freq_x = 0.00315;
    const double freq_z = 0.00315;

    for(int32_t x = 0; x < CHUNK_SIZE_X; ++x) {
        for(int32_t z = 0; z < CHUNK_SIZE_Z; ++z) {
            int32_t abs_x = x + CHUNK_SIZE_X * gen.chunk_xz.x;
            int32_t abs_z = z + CHUNK_SIZE_Z * gen.chunk_xz.y;
            double  noise = perlin.octave2D_01(abs_x * freq_x, abs_z * freq_z, octaves);

            struct Segment { 
                double left; 
                double right; 
                int32_t height_left; 
                int32_t height_right; 
            } segments[] = {
#if 0
                { 0.00, 0.25, 40 + 0,   40 + 6  },
                { 0.25, 0.30, 40 + 6,   40 + 14  },
                { 0.30, 0.65, 40 + 14,  40 + 80  },
                { 0.65, 0.68, 40 + 80,  40 + 96 },
                { 0.68, 0.75, 40 + 96,  40 + 128 },
                { 0.75, 0.80, 40 + 128, 40 + 164 },
                { 0.80, 0.90, 40 + 164, 40 + 196 },
                { 0.90, 1.00, 40 + 196, 40 + 200 },
#else
                { 0.00, 0.35, 40, 48 },
                { 0.35, 0.40, 48, 70 },
                { 0.40, 0.55, 70, 90 },
                { 0.55, 0.75, 90, 120 },
                { 0.75, 0.85, 120, 142 },
                { 0.85, 0.90, 142, 160 },
                { 0.90, 1.00, 160, 200 },
#endif
            };

            int32_t height = 0;
            for(int32_t index = 0; index < ARRAY_COUNT(segments); ++index) {
                Segment seg = segments[index];
                if(noise <= seg.right) {
                    double perc = (noise - seg.left) / (seg.right - seg.left);
                    height = lerp(seg.height_left, seg.height_right, perc);
                    break;
                }
            }
            height_map[x][z] = height;
        }
    }
}
