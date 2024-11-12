#include "chunk_mesh_gen.h"
#include "world.h"

static inline uint32_t get_block_array_index(const vec3i &block_rel) {
    return block_rel.x * (CHUNK_SIZE_Y * CHUNK_SIZE_Z) + block_rel.y * CHUNK_SIZE_Z + block_rel.z;
}

static inline Block get_block(ChunkMeshGenData &gen_data, const vec3i &block_rel) {
    return gen_data.chunk_blocks[get_block_array_index(block_rel)];
}

static constexpr void calc_atlas_tex_coords(int32_t x, int32_t y, ChunkVaoVertex v[4]) {
#if 0
    v[0].tex_coord = vec2{ x * 16.0f / 512.0f, y * 16.0f / 512.0f };
    v[1].tex_coord = vec2{ (x + 1) * 16.0f / 512.0f, y * 16.0f / 512.0f };
    v[2].tex_coord = vec2{ (x + 1) * 16.0f / 512.0f, (y + 1) * 16.0f / 512.0f };
    v[3].tex_coord = vec2{ x * 16.0f / 512.0f, (y + 1) * 16.0f / 512.0f };
#else

#endif
}


#if 0
void gen_single_block_vao_data(BlockType type, std::vector<ChunkVaoVertex> &vertices, std::vector<uint32_t> &indices) {
    bool opaques[BLOCK_SIDE_COUNT] = {
        false, false, false, 
        false, false, false
    };

    push_block_vao_data(type, vertices, indices, { 0, 0, 0 }, opaques);
    for(uint32_t index = 0; index < vertices.size(); ++index) {
        ChunkVaoVertex &v = vertices[index];
        v.position -= vec3::make(0.5f);
    }
}
#endif

static void fill_block_cube_vao_vertex(ChunkVaoVertex v[4], BlockSide side, const vec3i &offset) {
    switch(side) {
        default: INVALID_CODE_PATH; break;

        case BlockSide::Z_POS: {
            v[0].position = vec3{ 0.0f, 0.0f, 1.0f };
            v[1].position = vec3{ 1.0f, 0.0f, 1.0f };
            v[2].position = vec3{ 1.0f, 1.0f, 1.0f };
            v[3].position = vec3{ 0.0f, 1.0f, 1.0f };
            v[0].normal = vec3{ 0.0f, 0.0f, 1.0f };
            v[1].normal = vec3{ 0.0f, 0.0f, 1.0f };
            v[2].normal = vec3{ 0.0f, 0.0f, 1.0f };
            v[3].normal = vec3{ 0.0f, 0.0f, 1.0f };
        } break;

        case BlockSide::Z_NEG: {
            v[0].position = vec3{ 1.0f, 0.0f, 0.0f };
            v[1].position = vec3{ 0.0f, 0.0f, 0.0f };
            v[2].position = vec3{ 0.0f, 1.0f, 0.0f };
            v[3].position = vec3{ 1.0f, 1.0f, 0.0f };
            v[0].normal = vec3{ 0.0f, 0.0f, -1.0f };
            v[1].normal = vec3{ 0.0f, 0.0f, -1.0f };
            v[2].normal = vec3{ 0.0f, 0.0f, -1.0f };
            v[3].normal = vec3{ 0.0f, 0.0f, -1.0f };
        } break;

        case BlockSide::X_POS: {
            v[0].position = vec3{ 1.0f, 0.0f, 1.0f };
            v[1].position = vec3{ 1.0f, 0.0f, 0.0f };
            v[2].position = vec3{ 1.0f, 1.0f, 0.0f };
            v[3].position = vec3{ 1.0f, 1.0f, 1.0f };
            v[0].normal = vec3{ 1.0f, 0.0f, 0.0f };
            v[1].normal = vec3{ 1.0f, 0.0f, 0.0f };
            v[2].normal = vec3{ 1.0f, 0.0f, 0.0f };
            v[3].normal = vec3{ 1.0f, 0.0f, 0.0f };
        } break;

        case BlockSide::X_NEG: {
            v[0].position = vec3{ 0.0f, 0.0f, 0.0f };
            v[1].position = vec3{ 0.0f, 0.0f, 1.0f };
            v[2].position = vec3{ 0.0f, 1.0f, 1.0f };
            v[3].position = vec3{ 0.0f, 1.0f, 0.0f };
            v[0].normal = vec3{ -1.0f, 0.0f, 0.0f };
            v[1].normal = vec3{ -1.0f, 0.0f, 0.0f };
            v[2].normal = vec3{ -1.0f, 0.0f, 0.0f };
            v[3].normal = vec3{ -1.0f, 0.0f, 0.0f };
        } break;

        case BlockSide::Y_POS: {
            v[0].position = vec3{ 0.0f, 1.0f, 1.0f };
            v[1].position = vec3{ 1.0f, 1.0f, 1.0f };
            v[2].position = vec3{ 1.0f, 1.0f, 0.0f };
            v[3].position = vec3{ 0.0f, 1.0f, 0.0f };
            v[0].normal = vec3{ 0.0f, 1.0f, 0.0f };
            v[1].normal = vec3{ 0.0f, 1.0f, 0.0f };
            v[2].normal = vec3{ 0.0f, 1.0f, 0.0f };
            v[3].normal = vec3{ 0.0f, 1.0f, 0.0f };
        } break;

        case BlockSide::Y_NEG: {
            v[0].position = vec3{ 0.0f, 0.0f, 0.0f };
            v[1].position = vec3{ 1.0f, 0.0f, 0.0f };
            v[2].position = vec3{ 1.0f, 0.0f, 1.0f };
            v[3].position = vec3{ 0.0f, 0.0f, 1.0f };
            v[0].normal = vec3{ 0.0f, -1.0f, 0.0f };
            v[1].normal = vec3{ 0.0f, -1.0f, 0.0f };
            v[2].normal = vec3{ 0.0f, -1.0f, 0.0f };
            v[3].normal = vec3{ 0.0f, -1.0f, 0.0f };
        } break;
    }

    v[0].position += vec3::make(offset);
    v[1].position += vec3::make(offset);
    v[2].position += vec3::make(offset);
    v[3].position += vec3::make(offset);
}

static void fill_block_cube_vao_tex_coords(ChunkVaoVertex v[4], BlockType type, BlockSide side) {
    v[0].tex_coord = { 0.0f, 0.0f };
    v[1].tex_coord = { 1.0f, 0.0f };
    v[2].tex_coord = { 1.0f, 1.0f };
    v[3].tex_coord = { 0.0f, 1.0f };

    BlockTextureID texture_id = TEX_SAND;

    switch(type) {
        default: INVALID_CODE_PATH; break;

        case BlockType::SAND: {
            texture_id = TEX_SAND;
            calc_atlas_tex_coords(0, 0, v);
        } break;

        case BlockType::DIRT: {
            texture_id = TEX_DIRT;
            calc_atlas_tex_coords(1, 0, v);
        } break;

        case BlockType::DIRT_WITH_GRASS: {
            switch(side) {
                case BlockSide::Y_NEG: {
                    texture_id = TEX_DIRT;
                    calc_atlas_tex_coords(1, 0, v);
                } break;
                case BlockSide::Y_POS: {
                    calc_atlas_tex_coords(1, 2, v);
                    texture_id = TEX_DIRT_WITH_GRASS_TOP;
                } break;
                case BlockSide::X_POS:
                case BlockSide::X_NEG:
                case BlockSide::Z_POS:
                case BlockSide::Z_NEG: {
                    texture_id = TEX_DIRT_WITH_GRASS_SIDE;
                    calc_atlas_tex_coords(1, 1, v);
                } break;
            }
        } break;

        case BlockType::COBBLESTONE: {
            texture_id = TEX_COBBLESTONE;
            calc_atlas_tex_coords(2, 0, v);
        } break;

        case BlockType::STONE: {
            texture_id = TEX_STONE;
            calc_atlas_tex_coords(2, 1, v);
        } break;

        case BlockType::TREE_LOG: {
            switch(side) {
                case BlockSide::Y_POS:
                case BlockSide::Y_NEG: {
                    texture_id = TEX_TREE_LOG_TOP;
                    calc_atlas_tex_coords(7, 0, v);
                } break;

                case BlockSide::X_POS:
                case BlockSide::X_NEG:
                case BlockSide::Z_POS:
                case BlockSide::Z_NEG: {
                    texture_id = TEX_TREE_LOG_SIDE;
                    calc_atlas_tex_coords(7, 1, v);
                } break;
            };
        } break;

        case BlockType::TREE_LEAVES: {
            texture_id = TEX_TREE_LEAVES;
            calc_atlas_tex_coords(7, 2, v);
        } break;
        
        case BlockType::GLASS: {
            texture_id = TEX_GLASS;
            calc_atlas_tex_coords(0, 1, v);
        } break;

        case BlockType::WATER: {
            texture_id = TEX_WATER;
            calc_atlas_tex_coords(31, 0, v);
        } break;
    }

    v[0].tex_slot = (float)texture_id;
    v[1].tex_slot = (float)texture_id;
    v[2].tex_slot = (float)texture_id;
    v[3].tex_slot = (float)texture_id;
}

static void fill_block_cross_vao_vertex(ChunkVaoVertex v[2][4], const vec3i &offset) {
    v[0][0].position = vec3{ 0.0f, 0.0f, 0.0f };
    v[0][1].position = vec3{ 1.0f, 0.0f, 1.0f };
    v[0][2].position = vec3{ 1.0f, 1.0f, 1.0f };
    v[0][3].position = vec3{ 0.0f, 1.0f, 0.0f };

    v[1][0].position = vec3{ 0.0f, 0.0f, 1.0f };
    v[1][1].position = vec3{ 1.0f, 0.0f, 0.0f };
    v[1][2].position = vec3{ 1.0f, 1.0f, 0.0f };
    v[1][3].position = vec3{ 0.0f, 1.0f, 1.0f };

    v[0][0].position += vec3::make(offset);
    v[0][1].position += vec3::make(offset);
    v[0][2].position += vec3::make(offset);
    v[0][3].position += vec3::make(offset);

    v[1][0].position += vec3::make(offset);
    v[1][1].position += vec3::make(offset);
    v[1][2].position += vec3::make(offset);
    v[1][3].position += vec3::make(offset);

    v[0][0].normal = vec3::normalize({ 1.0f, 0.0f, 1.0f });
    v[0][1].normal = vec3::normalize({ 1.0f, 0.0f, 1.0f });
    v[0][2].normal = vec3::normalize({ 1.0f, 0.0f, 1.0f });
    v[0][3].normal = vec3::normalize({ 1.0f, 0.0f, 1.0f });

    v[1][0].normal = vec3::normalize({ 1.0f, 0.0f, -1.0f });
    v[1][1].normal = vec3::normalize({ 1.0f, 0.0f, -1.0f });
    v[1][2].normal = vec3::normalize({ 1.0f, 0.0f, -1.0f });
    v[1][3].normal = vec3::normalize({ 1.0f, 0.0f, -1.0f });
}

static void fill_block_cross_vao_tex_coords(ChunkVaoVertex v[2][4], BlockType type) {
    // @todo BlockTextureID texture_id = BlockTextureID::TEX_MISSING;
    
    v[0][0].tex_coord = { 0.0f, 0.0f };
    v[0][1].tex_coord = { 1.0f, 0.0f };
    v[0][2].tex_coord = { 1.0f, 1.0f };
    v[0][3].tex_coord = { 0.0f, 1.0f };

    v[1][0].tex_coord = { 0.0f, 0.0f };
    v[1][1].tex_coord = { 1.0f, 0.0f };
    v[1][2].tex_coord = { 1.0f, 1.0f };
    v[1][3].tex_coord = { 0.0f, 1.0f };

    BlockTextureID texture_id = TEX_SAND;

    switch(type) {
        default: break;

        case BlockType::GRASS: {
            texture_id = BlockTextureID::TEX_GRASS;
        } break;
    }

    v[0][0].tex_slot = (float)texture_id;
    v[0][1].tex_slot = (float)texture_id;
    v[0][2].tex_slot = (float)texture_id;
    v[0][3].tex_slot = (float)texture_id;

    v[1][0].tex_slot = (float)texture_id;
    v[1][1].tex_slot = (float)texture_id;
    v[1][2].tex_slot = (float)texture_id;
    v[1][3].tex_slot = (float)texture_id;
}

static void push_block_vao_quad(ChunkVaoVertex quad_vertices[4], ChunkMeshData &data) {
    const int32_t start = data.vertices.size();

    data.vertices.push_back(quad_vertices[0]);
    data.vertices.push_back(quad_vertices[1]);
    data.vertices.push_back(quad_vertices[2]);
    data.vertices.push_back(quad_vertices[3]);

    data.indices.push_back(start + 0);
    data.indices.push_back(start + 1);
    data.indices.push_back(start + 2);
    data.indices.push_back(start + 2);
    data.indices.push_back(start + 3);
    data.indices.push_back(start + 0);
}


static void push_water_vao_data(ChunkMeshData &data, const vec3i &block_rel, bool connect_wall[BLOCK_SIDE_COUNT]) {
    for(int32_t side = 0; side < BLOCK_SIDE_COUNT; ++side) {
        ChunkVaoVertex v[4] = { };

        if(!connect_wall[side]) {

            fill_block_cube_vao_vertex(v, (BlockSide)side, block_rel);

            v[0].tex_coord = { 0.0f, 0.0f };
            v[1].tex_coord = { 1.0f, 0.0f };
            v[2].tex_coord = { 1.0f, 1.0f };
            v[3].tex_coord = { 0.0f, 1.0f };

            /* Tex slot in water means water frame */
            v[0].tex_slot = 0.0f;
            v[1].tex_slot = 0.0f;
            v[2].tex_slot = 0.0f;
            v[3].tex_slot = 0.0f;

            const int32_t start = data.vertices.size();

            data.vertices.push_back(v[0]);
            data.vertices.push_back(v[1]);
            data.vertices.push_back(v[2]);
            data.vertices.push_back(v[3]);

            data.indices.push_back(start + 0);
            data.indices.push_back(start + 1);
            data.indices.push_back(start + 2);
            data.indices.push_back(start + 2);
            data.indices.push_back(start + 3);
            data.indices.push_back(start + 0);
        }
    }
}


static void push_block_vao_data(BlockType type, ChunkMeshData &data, const vec3i &block_rel, bool connect_wall[BLOCK_SIDE_COUNT]) {
    enum : int32_t { CUBE, CROSS, NOT_SET } block_vis;

    switch(type) {
        default: {
            INVALID_CODE_PATH;
        } break;

        case BlockType::SAND:
        case BlockType::DIRT:
        case BlockType::DIRT_WITH_GRASS:
        case BlockType::COBBLESTONE:
        case BlockType::STONE:
        case BlockType::TREE_LOG:
        case BlockType::TREE_LEAVES:
        case BlockType::GLASS:
        case BlockType::WATER: {
            block_vis = CUBE;
        } break;

        case BlockType::GRASS: {
            block_vis = CROSS;
        } break;
    }

    switch(block_vis) {
        default: {
            INVALID_CODE_PATH;
        } break;

        case CUBE: {
            for(int32_t side = 0; side < BLOCK_SIDE_COUNT; ++side) {
                ChunkVaoVertex v[4] = { };
                if(!connect_wall[side]) {
                    fill_block_cube_vao_vertex(v, (BlockSide)side, block_rel);
                    fill_block_cube_vao_tex_coords(v, type, (BlockSide)side);
                    push_block_vao_quad(v, data);
                }
            }
        } break;

        case CROSS: {
            ChunkVaoVertex v[2][4] = { };
            fill_block_cross_vao_vertex(v, block_rel);
            fill_block_cross_vao_tex_coords(v, type);
            push_block_vao_quad(v[0], data);
            push_block_vao_quad(v[1], data);
        } break;
    }
}

void chunk_mesh_gen_data_init(ChunkMeshGenData &gen_data, World &world, vec2i chunk_xz) {
    gen_data.chunk.vertices.clear();
    gen_data.water.vertices.clear();
    gen_data.chunk.indices.clear();
    gen_data.water.indices.clear();

    Chunk *chunk = world.get_chunk(chunk_xz);
    ASSERT(chunk);

    uint64_t chunk_bytes = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * sizeof(Block);

    gen_data.chunk_blocks = (Block *)malloc(chunk_bytes);

    memcpy(gen_data.chunk_blocks, chunk->get_block_data(), CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * sizeof(Block));
    gen_data.chunk_xz = chunk->get_chunk_xz();

    Chunk *chunk_z_neg = world.get_chunk(chunk_xz + vec2i{ 0, -1 });
    Chunk *chunk_z_pos = world.get_chunk(chunk_xz + vec2i{ 0,  1 });
    Chunk *chunk_x_neg = world.get_chunk(chunk_xz + vec2i{-1,  0 });
    Chunk *chunk_x_pos = world.get_chunk(chunk_xz + vec2i{ 1,  0 });

    if(chunk_z_neg) {
        gen_data.chunk_z_neg = (Block *)malloc(chunk_bytes);
        memcpy(gen_data.chunk_z_neg, chunk_z_neg->get_block_data(), chunk_bytes);
    } else {
        gen_data.chunk_z_neg = NULL;
    }

    if(chunk_z_pos) {
        gen_data.chunk_z_pos = (Block *)malloc(chunk_bytes);
        memcpy(gen_data.chunk_z_pos, chunk_z_pos->get_block_data(), CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * sizeof(Block));
    } else {
        gen_data.chunk_z_pos = NULL;
    }

    if(chunk_x_neg) {
        gen_data.chunk_x_neg = (Block *)malloc(chunk_bytes);
        memcpy(gen_data.chunk_x_neg, chunk_x_neg->get_block_data(), CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * sizeof(Block));
    } else {
        gen_data.chunk_x_neg = NULL;
    }

    if(chunk_x_pos) {
        gen_data.chunk_x_pos = (Block *)malloc(chunk_bytes);
        memcpy(gen_data.chunk_x_pos, chunk_x_pos->get_block_data(), CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * sizeof(Block));
    } else {
        gen_data.chunk_x_pos = NULL;
    }
}

void chunk_mesh_gen_data_free(ChunkMeshGenData &gen_data) {
    free(gen_data.chunk_blocks);
    if(gen_data.chunk_z_neg) {
        free(gen_data.chunk_z_neg);
    }
    if(gen_data.chunk_z_pos) {
        free(gen_data.chunk_z_pos);
    }
    if(gen_data.chunk_x_neg) {
        free(gen_data.chunk_x_neg);
    }
    if(gen_data.chunk_x_pos) {
        free(gen_data.chunk_x_pos);
    }
}

void chunk_mesh_gen(ChunkMeshGenData &gen_data) {
    for_every_block(x, y, z) {
        vec3i block_rel = { x, y, z };
        Block block = get_block(gen_data, block_rel);

        /* Water goes to different vao */
        if(block.type == BlockType::WATER) {
            continue;
        }

        const uint32_t flags = get_block_flags(block.type);
        if(flags & IS_NOT_VISIBLE) {
            continue;
        }

        bool connect_wall[BLOCK_SIDE_COUNT] = { };

        for(int32_t side = 0; side < BLOCK_SIDE_COUNT; ++side) {
            vec3i nb_block_rel = block_rel + get_block_side_dir((BlockSide)side);

            Block nb_block = { .type = BlockType::AIR };
            if(!is_inside_chunk(nb_block_rel)) {
                BlockSide side_e = (BlockSide)side;

                if(side_e == BlockSide::Y_NEG || side_e == BlockSide::Y_POS) {
                    continue;
                }

                if(side_e == BlockSide::Z_NEG) {
                    if(gen_data.chunk_z_neg) {
                        nb_block_rel.z = CHUNK_SIZE_Z - 1;
                        nb_block = gen_data.chunk_z_neg[get_block_array_index(nb_block_rel)];
                    } else {
                        continue;
                    }
                } else if(side_e == BlockSide::Z_POS) {
                    if(gen_data.chunk_z_pos) {
                        nb_block_rel.z = 0;
                        nb_block = gen_data.chunk_z_pos[get_block_array_index(nb_block_rel)];
                    } else {
                        continue;
                    }
                } else if(side_e == BlockSide::X_NEG) {
                    if(gen_data.chunk_x_neg) {
                        nb_block_rel.x = CHUNK_SIZE_X - 1;
                        nb_block = gen_data.chunk_x_neg[get_block_array_index(nb_block_rel)];
                    } else {
                        continue;
                    }
                } else if(side_e == BlockSide::X_POS) {
                    if(gen_data.chunk_x_pos) {
                        nb_block_rel.x = 0;
                        nb_block = gen_data.chunk_x_pos[get_block_array_index(nb_block_rel)];
                    } else {
                        continue;
                    }
                }

            } else {
                nb_block = get_block(gen_data, nb_block_rel);
            }

            uint32_t nb_flags = get_block_flags(nb_block.type);

            connect_wall[side] = !(nb_flags & IS_NOT_VISIBLE) && !(nb_flags & HAS_TRANSPARENCY);

            if(nb_block.type == BlockType::WATER) {
                connect_wall[side] = false;
            }

            if(flags & HAS_TRANSPARENCY && nb_flags & HAS_TRANSPARENCY && !(nb_flags & IS_NOT_VISIBLE)) {
                connect_wall[side] = true;
            }
        }

        push_block_vao_data(block.type, gen_data.chunk, block_rel, connect_wall);
    }
}

/* @TODO */
ChunkMeshData chunk_mesh_gen_single_block(BlockType type) {
    ChunkMeshData mesh_data = { };

    bool connect_wall[BLOCK_SIDE_COUNT] = {
        false, false, false, 
        false, false, false
    };

    push_block_vao_data(type, mesh_data, { 0, 0, 0 }, connect_wall);
    for(uint32_t index = 0; index < mesh_data.vertices.size(); ++index) {
        ChunkVaoVertex &v = mesh_data.vertices[index];
        v.position -= vec3::make(0.5f);
    }

    return mesh_data;
}
