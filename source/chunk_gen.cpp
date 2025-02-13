#include "chunk_gen.h"

// https://github.com/Reputeless/PerlinNoise
#include <PerlinNoise.hpp>

static inline BlockType get_block(ChunkGenData &gen, const vec3i &block_rel) {
    return gen.blocks[get_block_array_index(block_rel)];
}

static inline void set_block(ChunkGenData &gen, const vec3i &block_rel, BlockType type) {
    ASSERT(is_block_type_valid(type));
    gen.blocks[get_block_array_index(block_rel)] = type;
}

void chunk_gen_data_init(ChunkGenData &gen, vec2i chunk_xz, WorldGenSeed seed) {
    gen.chunk_xz = chunk_xz;
    gen.seed = seed;
}

static inline bool should_plant_flower(ChunkGenData &gen, int32_t rel_x, int32_t rel_z, BlockType &to_plant) {
    const uint32_t seed = gen.chunk_xz.x * 32 + gen.chunk_xz.y * 17 + rel_x * 7 + rel_z * 13 + gen.seed.seed;
    srand(seed);
    if((rand() % 35) == 0) {
        int32_t plant_choice = rand() % 100;
        if(plant_choice <= 60) {
            to_plant = BlockType::GRASS;
        } else if(plant_choice <= 80) {
            to_plant = BlockType::DANDELION;
        } else {
            to_plant = BlockType::ROSE;
        }
        return true;
    }
    return false;
}

static inline bool should_plant_tree(ChunkGenData &gen, int32_t rel_x, int32_t rel_z, uint8_t biome, BlockType &to_plant) {
    const uint32_t seed = gen.chunk_xz.x * 32 + gen.chunk_xz.y * 17 + rel_x * 7 + rel_z * 13 + gen.seed.seed;
    srand(seed);

    int32_t tree_chance = 1000;
    switch(biome) {
        case BIOME_FOREST: {
            tree_chance = 150;
        } break;

        case BIOME_PLAINS: {
            tree_chance = 700;
        } break;
    }

    if((rand() % tree_chance) == 0) {
        int32_t tree_choice = rand() % 4;
        if(tree_choice == 0) {
            to_plant = BlockType::TREE_BIRCH_LOG; /* 1/4 chance for birch */
        } else {
            to_plant = BlockType::TREE_OAK_LOG;   /* 3/4 chance for oak */
        }
        return true;
    }

    return false;
}

static inline bool should_plant_cactus(ChunkGenData &gen, int32_t rel_x, int32_t rel_z) {
    const uint32_t seed = gen.chunk_xz.x * 32 + gen.chunk_xz.y * 17 + rel_x * 7 + rel_z * 13 + gen.seed.seed;
    srand(seed);
    return (rand() % 200) == 0;
}

static inline bool should_plant_deadbush(ChunkGenData &gen, int32_t rel_x, int32_t rel_z) {
    const uint32_t seed = gen.chunk_xz.x * 14 + gen.chunk_xz.y * 13 + rel_x * 17 + rel_z * 11 + gen.seed.seed;
    srand(seed);
    return (rand() % 170) == 0;
}

static void try_insert_blocks(ChunkGenData &gen, const vec3i block_origin, BlockOffset *blocks, int32_t count) {
    // @TODO : Stupid first pass to check if tree would be inside chunk's bounds, To change (blocks should be able to be to cross chunk)
    for(int32_t index = 0; index < count; ++index) {
        const BlockOffset block = blocks[index];
        const vec3i block_rel = block_origin + block.offset;
        if(!is_inside_chunk(block_rel)) {
            return; /* Do not insert blocks */
        }
    }

    for(int32_t index = 0; index < count; ++index) {
        const BlockOffset block = blocks[index];
        const vec3i block_rel = block_origin + block.offset;
        if(get_block(gen, block_rel) == BlockType::AIR) {
            set_block(gen, block_rel, block.type);
        }
    }
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
                            // int32_t rand_block = rand() % 2;
                            // BlockType block_to_place = rand_block == 0 ? BlockType::STONE : BlockType::DIRT;
                            BlockType block_to_place = BlockType::STONE;
                            set_block(gen, vec3i{ x, y, z }, block_to_place);
                        } break;
                    }
                }
            }

            const vec3i block_origin = { x, height_map[x][z], z };

            /* @TEMP : Basic structure inserting */
            if(height_map[x][z] > ocean_level) {
                switch(biome_map[x][z]) {
                    case BIOME_PLAINS:
                    case BIOME_FOREST: {
                        
                        BlockType tree_to_plant = BlockType::_INVALID;
                        if(should_plant_tree(gen, x, z, biome_map[x][z], tree_to_plant)) {
                            switch(tree_to_plant) {
                                default: ASSERT(false); break;

                                case BlockType::TREE_OAK_LOG: {
                                    try_insert_blocks(gen, block_origin, oak_tree_blocks, ARRAY_COUNT(oak_tree_blocks));
                                } break;

                                case BlockType::TREE_BIRCH_LOG: {
                                    try_insert_blocks(gen, block_origin, birch_tree_blocks, ARRAY_COUNT(birch_tree_blocks));
                                } break;
                            }
                        }

                        BlockType flower_to_plant = BlockType::_INVALID;
                        if(should_plant_flower(gen, x, z, flower_to_plant)) {
                            const vec3i block_dest = { x, height_map[x][z], z };
                            if(get_block(gen, block_dest) == BlockType::AIR) {
                                set_block(gen, block_dest, flower_to_plant);
                            }
                        }
                    } break;

                    case BIOME_DESERT: {
                        if(should_plant_cactus(gen, x, z)) {
                            try_insert_blocks(gen, block_origin, cactus_blocks, ARRAY_COUNT(cactus_blocks));
                        }

                        if(should_plant_deadbush(gen, x, z)) {
                            if(height_map[x][z] + 1 < CHUNK_SIZE_Y) {
                                if(get_block(gen, block_origin) == BlockType::AIR) {
                                    set_block(gen, block_origin, BlockType::DEADBUSH);
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
                { 0.00, 0.35, 40, 58 },
                { 0.35, 0.40, 58, 67 },
                { 0.40, 0.55, 67, 90 },
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
