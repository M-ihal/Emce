#include "chunk_gen.h"
#include <PerlinNoise.hpp>

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

    for(int32_t x = 0; x < CHUNK_SIZE_X; ++x) {
        for(int32_t z = 0; z < CHUNK_SIZE_Z; ++z) {

            /* Fill structure of the chunk */
            for(int32_t y = 0; y < height_map[x][z]; ++y) {
                set_block(gen, vec3i{ x, y, z }, BlockType::STONE);
            }

            if(rand() % 3 == 0) {
                set_block(gen, vec3i{ x, height_map[x][z], z }, BlockType::GRASS);
            }
        }
    }
}

void chunk_gen_height_map(ChunkGenData &gen, int32_t height_map[CHUNK_SIZE_X][CHUNK_SIZE_Z]) {
    siv::PerlinNoise perlin(gen.seed.seed);

    const int32_t octaves = 4;
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
                { 0.00, 0.25, 40 + 0,   40 + 16  },
                { 0.25, 0.30, 40 + 16,  40 + 32  },
                { 0.30, 0.65, 40 + 32,  40 + 80  },
                { 0.65, 0.68, 40 + 80,  40 + 96 },
                { 0.68, 0.75, 40 + 96,  40 + 128 },
                { 0.75, 0.80, 40 + 128, 40 + 164 },
                { 0.80, 0.90, 40 + 164, 40 + 196 },
                { 0.90, 1.00, 40 + 196, 40 + 200 },
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
