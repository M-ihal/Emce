#include "chunk_gen.h"
#include <PerlinNoise.hpp>

static inline uint32_t get_block_array_index(const vec3i &block_rel) {
    return block_rel.x * (CHUNK_SIZE_Y * CHUNK_SIZE_Z) + block_rel.y * CHUNK_SIZE_Z + block_rel.z;
}

static inline BlockType get_block(ChunkGenData &gen, const vec3i &block_rel) {
    return gen.blocks[get_block_array_index(block_rel)];
}

static inline void set_block(ChunkGenData &gen, const vec3i &block_rel, BlockType type) {
    gen.blocks[get_block_array_index(block_rel)] = type;
}

void chunk_gen_data_init(ChunkGenData &gen, vec2i chunk_xz, WorldGenSeed seed) {
    gen.chunk_xz = chunk_xz;
    gen.seed = seed;
}

/* @TODO : Change rand() ... */

void chunk_gen(ChunkGenData &gen) {
    /* Clear chunk to 0 (AIR) */
    memset(gen.blocks, (uint8_t)BlockType::AIR, CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * sizeof(BlockType));

    int32_t height_map[CHUNK_SIZE_X][CHUNK_SIZE_Z];
    chunk_gen_height_map(gen, height_map);

    int32_t biome_map[CHUNK_SIZE_X][CHUNK_SIZE_Z];
    chunk_gen_biome_map(gen, biome_map);

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
                if(y <= ocean_level) {
                    set_block(gen, vec3i{ x, y, z }, BlockType::SAND);
                } else {
                    switch(biome_map[x][z]) {
                        case BIOME_PLAINS:
                        case BIOME_FOREST: {
                            if(y == height_map[x][z] - 1) {
                                set_block(gen, vec3i{ x, y, z }, BlockType::DIRT_WITH_GRASS);
                            } else {
                                set_block(gen, vec3i{ x, y, z }, BlockType::DIRT);
                            }
                        } break;

                        case BIOME_DESERT: {
                            set_block(gen, vec3i{ x, y, z }, BlockType::SAND);
                        } break;

                        case BIOME_STONE: {
                            int32_t rand_block = rand() % 2;
                            BlockType block_to_place = rand_block == 0 ? BlockType::STONE : BlockType::DIRT;
                            set_block(gen, vec3i{ x, y, z }, block_to_place);
                        } break;
                    }
                }
            }

            /* @TEMP : Basic structure inserting */
            if(height_map[x][z] > ocean_level) {
                switch(biome_map[x][z]) {
                    case BIOME_PLAINS:
                    case BIOME_FOREST: {
                        /* Plant foliage */
                        if(rand() % 50 == 0) {
                            BlockType to_plant;
                            int plant_rand = rand() % 200;
                            if(plant_rand < 120) {
                                to_plant = BlockType::GRASS;
                            } else if(plant_rand < 160) {
                                to_plant = BlockType::DANDELION;
                            } else {
                                to_plant = BlockType::ROSE;
                            }

                            if(get_block(gen, vec3i{ x, height_map[x][z], z }) == BlockType::AIR) {
                                set_block(gen, vec3i{ x, height_map[x][z], z }, to_plant);
                            }
                        }

                        /* Plant trees */
                        int32_t plant_tree_chance = biome_map[x][z] == BIOME_FOREST ? 140 : 750;
                        bool plant_tree = rand() % plant_tree_chance == 0;
                        if(plant_tree) {
                            if(x >= 2 && x < CHUNK_SIZE_X - 2 && z >= 2 && z < CHUNK_SIZE_Z - 2 && (height_map[x][z] + 6) < CHUNK_SIZE_Y) {
                                int32_t tree_type_chance = rand() % 4;
                                if(tree_type_chance == 0) {
                                    /* Birch 1/4 */
                                    for(int32_t index = 0; index < ARRAY_COUNT(birch_tree_blocks); ++index) {
                                        BlockOffset block = birch_tree_blocks[index];

                                        vec3i block_rel = vec3i{ x, height_map[x][z], z } + block.offset;
                                        if(get_block(gen, block_rel) == BlockType::AIR) {
                                            set_block(gen, block_rel, block.type);
                                        }
                                    }
                                } else {
                                    /* Oak */
                                    for(int32_t index = 0; index < ARRAY_COUNT(oak_tree_blocks); ++index) {
                                        BlockOffset block = oak_tree_blocks[index];

                                        vec3i block_rel = vec3i{ x, height_map[x][z], z } + block.offset;
                                        if(get_block(gen, block_rel) == BlockType::AIR) {
                                            set_block(gen, block_rel, block.type);
                                        }
                                    }
                                }
                            }
                        }
                    } break;

                    case BIOME_DESERT: {
                        /* Plant cacti */
                        if(rand() % 200 == 0) {
                            if(height_map[x][z] + 2 < CHUNK_SIZE_Y) {
                                for(int32_t index = 0; index < ARRAY_COUNT(cactus_blocks); ++index) {
                                    BlockOffset block = cactus_blocks[index];

                                    vec3i block_rel = vec3i{ x, height_map[x][z], z } + block.offset;
                                    if(get_block(gen, block_rel) == BlockType::AIR) {
                                        set_block(gen, block_rel, block.type);
                                    }
                                }
                            }
                        }

                        /* Plant deadbush */
                        if(rand() % 200 == 0) {
                            if(height_map[x][z] + 1 < CHUNK_SIZE_Y) {
                                vec3i block_rel = { x, height_map[x][z], z };
                                if(get_block(gen, block_rel) == BlockType::AIR) {
                                    set_block(gen, block_rel, BlockType::DEADBUSH);
                                }
                            }
                        }
                    } break;
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
                { 0.00, 0.35, 40, 48 },
                { 0.35, 0.40, 48, 70 },
                { 0.40, 0.55, 70, 90 },
                { 0.55, 0.75, 90, 120 },
                { 0.75, 0.85, 120, 142 },
                { 0.85, 0.90, 142, 160 },
                { 0.90, 1.00, 160, 220 },
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

void chunk_gen_biome_map(ChunkGenData &gen, int32_t biome_map[CHUNK_SIZE_X][CHUNK_SIZE_Z]) {
    siv::PerlinNoise perlin(gen.seed.seed + 2);

    const int32_t octaves = 7;
    const double freq_x = 0.00315;
    const double freq_z = 0.00315;

    for(int32_t x = 0; x < CHUNK_SIZE_X; ++x) {
        for(int32_t z = 0; z < CHUNK_SIZE_Z; ++z) {
            int32_t abs_x = x + CHUNK_SIZE_X * gen.chunk_xz.x;
            int32_t abs_z = z + CHUNK_SIZE_Z * gen.chunk_xz.y;
            double noise = perlin.octave2D_01(abs_x * freq_x, abs_z * freq_z, octaves);

            int32_t biome;
            if(noise < 0.2) {
                biome = BIOME_DESERT;
            } else if(noise < 0.6) {
                biome = BIOME_PLAINS;
            } else if(noise < 0.9) {
                biome = BIOME_FOREST;
            } else {
                biome = BIOME_STONE;
            }
            biome_map[x][z] = biome;
        }
    }
}
