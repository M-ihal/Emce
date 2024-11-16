#include "chunk_mesh_gen.h"
#include "world.h"

static inline uint32_t get_block_array_index(const vec3i &block_rel) {
    return block_rel.x * (CHUNK_SIZE_Y * CHUNK_SIZE_Z) + block_rel.y * CHUNK_SIZE_Z + block_rel.z;
}

static inline Block get_block(ChunkMeshGenData *gen_data, const vec3i &block_rel) {
    return gen_data->chunk_blocks[get_block_array_index(block_rel)];
}

static inline void fill_tex_coords(ChunkVaoVertex v[4]) {
    v[0].tex_coord = { 0.0f, 0.0f };
    v[1].tex_coord = { 1.0f, 0.0f };
    v[2].tex_coord = { 1.0f, 1.0f };
    v[3].tex_coord = { 0.0f, 1.0f };
}

static inline void fill_tex_slots(ChunkVaoVertex v[4], uint32_t slot) {
    v[0].tex_slot = (float)slot;
    v[1].tex_slot = (float)slot;
    v[2].tex_slot = (float)slot;
    v[3].tex_slot = (float)slot;
}

static inline void offset_vertices(ChunkVaoVertex v[4], const vec3i &offset) {
    vec3 offset_f = vec3::make(offset);
    v[0].position += offset_f;
    v[1].position += offset_f;
    v[2].position += offset_f;
    v[3].position += offset_f;
}

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

    offset_vertices(v, offset);
}

static void fill_block_cube_vao_tex_coords(ChunkVaoVertex v[4], BlockType type, BlockSide side) {
    BlockTextureID texture_id = TEX_SAND;
    switch(type) {
        default: INVALID_CODE_PATH; break;

        case BlockType::SAND: {
            texture_id = TEX_SAND;
        } break;

        case BlockType::DIRT: {
            texture_id = TEX_DIRT;
        } break;

        case BlockType::DIRT_WITH_GRASS: {
            switch(side) {
                case BlockSide::Y_NEG: {
                    texture_id = TEX_DIRT;
                } break;
                case BlockSide::Y_POS: {
                    texture_id = TEX_DIRT_WITH_GRASS_TOP;
                } break;
                case BlockSide::X_POS:
                case BlockSide::X_NEG:
                case BlockSide::Z_POS:
                case BlockSide::Z_NEG: {
                    texture_id = TEX_DIRT_WITH_GRASS_SIDE;
                } break;
            }
        } break;

        case BlockType::COBBLESTONE: {
            texture_id = TEX_COBBLESTONE;
        } break;

        case BlockType::STONE: {
            texture_id = TEX_STONE;
        } break;

        case BlockType::TREE_LOG: {
            switch(side) {
                case BlockSide::Y_POS:
                case BlockSide::Y_NEG: {
                    texture_id = TEX_TREE_LOG_TOP;
                } break;

                case BlockSide::X_POS:
                case BlockSide::X_NEG:
                case BlockSide::Z_POS:
                case BlockSide::Z_NEG: {
                    texture_id = TEX_TREE_LOG_SIDE;
                } break;
            };
        } break;

        case BlockType::TREE_LEAVES: {
            texture_id = TEX_TREE_LEAVES;
        } break;
        
        case BlockType::GLASS: {
            texture_id = TEX_GLASS;
        } break;

        case BlockType::WATER: {
            texture_id = TEX_WATER;
        } break;
    }

    fill_tex_coords(v);
    fill_tex_slots(v, texture_id);
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

    v[0][0].normal = vec3::normalize({ 1.0f, 0.0f, 1.0f });
    v[0][1].normal = vec3::normalize({ 1.0f, 0.0f, 1.0f });
    v[0][2].normal = vec3::normalize({ 1.0f, 0.0f, 1.0f });
    v[0][3].normal = vec3::normalize({ 1.0f, 0.0f, 1.0f });

    v[1][0].normal = vec3::normalize({ 1.0f, 0.0f, -1.0f });
    v[1][1].normal = vec3::normalize({ 1.0f, 0.0f, -1.0f });
    v[1][2].normal = vec3::normalize({ 1.0f, 0.0f, -1.0f });
    v[1][3].normal = vec3::normalize({ 1.0f, 0.0f, -1.0f });

    offset_vertices(v[0], offset);
    offset_vertices(v[1], offset);
}

static void fill_block_cross_vao_tex_coords(ChunkVaoVertex v[2][4], BlockType type) {
    BlockTextureID texture_id = TEX_SAND;
    switch(type) {
        default: break;

        case BlockType::GRASS: {
            texture_id = BlockTextureID::TEX_GRASS;
        } break;
    }

    fill_tex_coords(v[0]);
    fill_tex_coords(v[1]);
    fill_tex_slots(v[0], texture_id);
    fill_tex_slots(v[1], texture_id);
}

static void push_vao_quad(ChunkVaoVertex quad_vertices[4], ChunkMeshData &data) {
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
            fill_tex_coords(v);
            fill_tex_slots(v, 0); /* Tex slot in water means water frame */
            push_vao_quad(v, data);
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
                    push_vao_quad(v, data);
                }
            }
        } break;

        case CROSS: {
            ChunkVaoVertex v[2][4] = { };
            fill_block_cross_vao_vertex(v, block_rel);
            fill_block_cross_vao_tex_coords(v, type);
            push_vao_quad(v[0], data);
            push_vao_quad(v[1], data);
        } break;
    }
}

void chunk_mesh_gen_data_init(ChunkMeshGenData **gen_data_ptr, World &world, vec2i chunk_xz) {
    *gen_data_ptr = new ChunkMeshGenData;

    ChunkMeshGenData *gen_data = *gen_data_ptr;
    ASSERT(gen_data);

    Chunk *chunk = world.get_chunk(chunk_xz);
    Chunk *chunk_z_neg = world.get_chunk(chunk_xz + vec2i{ 0, -1 });
    Chunk *chunk_z_pos = world.get_chunk(chunk_xz + vec2i{ 0,  1 });
    Chunk *chunk_x_neg = world.get_chunk(chunk_xz + vec2i{-1,  0 });
    Chunk *chunk_x_pos = world.get_chunk(chunk_xz + vec2i{ 1,  0 });

    /* Meshing should only appear if neighbouring blocks exist */
    ASSERT(chunk && chunk_x_neg && chunk_x_pos && chunk_z_neg && chunk_z_pos);

    const size_t chunk_blocks_size = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * sizeof(Block);

    /* Copy the main chunk's blocks */
    gen_data->chunk_xz = chunk->get_chunk_xz();
    memcpy(gen_data->chunk_blocks, chunk->get_block_data(), chunk_blocks_size);

    /* Copy the neighbouring chunks' blocks */
    memcpy(gen_data->chunk_x_neg, chunk_x_neg->get_block_data(), chunk_blocks_size);
    memcpy(gen_data->chunk_x_pos, chunk_x_pos->get_block_data(), chunk_blocks_size);
    memcpy(gen_data->chunk_z_neg, chunk_z_neg->get_block_data(), chunk_blocks_size);
    memcpy(gen_data->chunk_z_pos, chunk_z_pos->get_block_data(), chunk_blocks_size);
}

void chunk_mesh_gen_data_free(ChunkMeshGenData **gen_data_ptr) {
    if(*gen_data_ptr != NULL) {
        delete *gen_data_ptr;
        *gen_data_ptr = NULL;
    }
}

void chunk_mesh_gen(ChunkMeshGenData *gen_data) {
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
                    nb_block_rel.z = CHUNK_SIZE_Z - 1;
                    nb_block = gen_data->chunk_z_neg[get_block_array_index(nb_block_rel)];
                } else if(side_e == BlockSide::Z_POS) {
                    nb_block_rel.z = 0;
                    nb_block = gen_data->chunk_z_pos[get_block_array_index(nb_block_rel)];
                } else if(side_e == BlockSide::X_NEG) {
                    nb_block_rel.x = CHUNK_SIZE_X - 1;
                    nb_block = gen_data->chunk_x_neg[get_block_array_index(nb_block_rel)];
                } else if(side_e == BlockSide::X_POS) {
                    nb_block_rel.x = 0;
                    nb_block = gen_data->chunk_x_pos[get_block_array_index(nb_block_rel)];
                }

            } else {
                nb_block = get_block(gen_data, nb_block_rel);
            }

            uint32_t nb_flags = get_block_flags(nb_block.type);

            connect_wall[side] = !(nb_flags & IS_NOT_VISIBLE) && !(nb_flags & HAS_TRANSPARENCY);

            if(nb_block.type == BlockType::WATER) {
                connect_wall[side] = false;
            } else {
                if(flags & HAS_TRANSPARENCY && nb_flags & HAS_TRANSPARENCY && !(nb_flags & IS_NOT_VISIBLE)) {
                    connect_wall[side] = true;
                }
            }
        }

        push_block_vao_data(block.type, gen_data->chunk, block_rel, connect_wall);
    }

    /* Generate water vao data */
    for_every_block(x, y, z) {
        const vec3i block_rel = { x, y, z };

        Block block = get_block(gen_data, block_rel);
        if(block.type != BlockType::WATER) {
            continue;
        }

        bool connect_wall[8] = { };
        for(int32_t side = 0; side < BLOCK_SIDE_COUNT; ++side) {
            vec3i nb_block_rel = block_rel + get_block_side_dir((BlockSide)side);

            Block nb_block = { .type = BlockType::AIR };
            if(!is_inside_chunk(nb_block_rel)) {
                BlockSide side_e = (BlockSide)side;

                if(side_e == BlockSide::Y_NEG || side_e == BlockSide::Y_POS) {
                    continue;
                }

                if(side_e == BlockSide::Z_NEG) {
                    nb_block_rel.z = CHUNK_SIZE_Z - 1;
                    nb_block = gen_data->chunk_z_neg[get_block_array_index(nb_block_rel)];
                } else if(side_e == BlockSide::Z_POS) {
                    nb_block_rel.z = 0;
                    nb_block = gen_data->chunk_z_pos[get_block_array_index(nb_block_rel)];
                } else if(side_e == BlockSide::X_NEG) {
                    nb_block_rel.x = CHUNK_SIZE_X - 1;
                    nb_block = gen_data->chunk_x_neg[get_block_array_index(nb_block_rel)];
                } else if(side_e == BlockSide::X_POS) {
                    nb_block_rel.x = 0;
                    nb_block = gen_data->chunk_x_pos[get_block_array_index(nb_block_rel)];
                }

            } else {
                nb_block = get_block(gen_data, nb_block_rel);
            }

            if(nb_block.type != BlockType::AIR) {
                uint32_t nb_flags = get_block_flags(nb_block.type);
                connect_wall[side] = !(nb_flags & IS_NOT_VISIBLE) && !(nb_flags & HAS_TRANSPARENCY);


                if(nb_block.type == BlockType::WATER) {
                    connect_wall[side] = true;
                }
            }
        }

        push_water_vao_data(gen_data->water, block_rel, connect_wall);
    }
}

/* @TODO */
void chunk_mesh_gen_single_block(ChunkMeshData &mesh_data, BlockType type) {
    bool connect_wall[BLOCK_SIDE_COUNT] = {
        false, false, false, 
        false, false, false
    };

    push_block_vao_data(type, mesh_data, { 0, 0, 0 }, connect_wall);
    for(uint32_t index = 0; index < mesh_data.vertices.size(); ++index) {
        ChunkVaoVertex &v = mesh_data.vertices[index];
        v.position -= vec3::make(0.5f);
    }
}
