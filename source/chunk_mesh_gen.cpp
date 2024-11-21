#include "chunk_mesh_gen.h"
#include "world.h"

static inline uint32_t get_block_array_index(const vec3i &block_rel) {
    return block_rel.x * (CHUNK_SIZE_Y * CHUNK_SIZE_Z) + block_rel.y * CHUNK_SIZE_Z + block_rel.z;
}

static inline Block get_block(ChunkMeshGenData *gen_data, const vec3i &block_rel) {
    return gen_data->chunk_blocks[get_block_array_index(block_rel)];
}

static inline void fill_tex_coords(ChunkVaoVertex v[4]) {
    v[0].tc = 0;
    v[1].tc = 1;
    v[2].tc = 2;
    v[3].tc = 3;
}

static inline void fill_tex_slots(ChunkVaoVertex v[4], uint8_t slot) {
    v[0].ts = slot;
    v[1].ts = slot;
    v[2].ts = slot;
    v[3].ts = slot;
}

static inline void offset_vertices(ChunkVaoVertex v[4], const vec3i &offset) {
    v[0].x += offset.x; v[0].y += offset.y; v[0].z += offset.z;
    v[1].x += offset.x; v[1].y += offset.y; v[1].z += offset.z;
    v[2].x += offset.x; v[2].y += offset.y; v[2].z += offset.z;
    v[3].x += offset.x; v[3].y += offset.y; v[3].z += offset.z;
}

static void fill_block_cube_vao_vertex(ChunkVaoVertex v[4], BlockSide side, const vec3i &offset) {
    switch(side) {
        default: INVALID_CODE_PATH; break;

        case BlockSide::X_NEG: {
            v[0].x = 0; v[0].y = 0; v[0].z = 0; v[0].n = 0;
            v[1].x = 0; v[1].y = 0; v[1].z = 1; v[1].n = 0;
            v[2].x = 0; v[2].y = 1; v[2].z = 1; v[2].n = 0;
            v[3].x = 0; v[3].y = 1; v[3].z = 0; v[3].n = 0;
        } break;

        case BlockSide::X_POS: {
            v[0].x = 1; v[0].y = 0; v[0].z = 1; v[0].n = 1;
            v[1].x = 1; v[1].y = 0; v[1].z = 0; v[1].n = 1;
            v[2].x = 1; v[2].y = 1; v[2].z = 0; v[2].n = 1;
            v[3].x = 1; v[3].y = 1; v[3].z = 1; v[3].n = 1;
        } break;

        case BlockSide::Z_NEG: {
            v[0].x = 1; v[0].y = 0; v[0].z = 0; v[0].n = 2;
            v[1].x = 0; v[1].y = 0; v[1].z = 0; v[1].n = 2;
            v[2].x = 0; v[2].y = 1; v[2].z = 0; v[2].n = 2;
            v[3].x = 1; v[3].y = 1; v[3].z = 0; v[3].n = 2;
        } break;

        case BlockSide::Z_POS: {
            v[0].x = 0; v[0].y = 0; v[0].z = 1; v[0].n = 3;
            v[1].x = 1; v[1].y = 0; v[1].z = 1; v[1].n = 3;
            v[2].x = 1; v[2].y = 1; v[2].z = 1; v[2].n = 3;
            v[3].x = 0; v[3].y = 1; v[3].z = 1; v[3].n = 3;
        } break;

        case BlockSide::Y_NEG: {
            v[0].x = 0; v[0].y = 0; v[0].z = 0; v[0].n = 4;
            v[1].x = 1; v[1].y = 0; v[1].z = 0; v[1].n = 4;
            v[2].x = 1; v[2].y = 0; v[2].z = 1; v[2].n = 4;
            v[3].x = 0; v[3].y = 0; v[3].z = 1; v[3].n = 4;
        } break;

        case BlockSide::Y_POS: {
            v[0].x = 0; v[0].y = 1; v[0].z = 1; v[0].n = 5;
            v[1].x = 1; v[1].y = 1; v[1].z = 1; v[1].n = 5;
            v[2].x = 1; v[2].y = 1; v[2].z = 0; v[2].n = 5;
            v[3].x = 0; v[3].y = 1; v[3].z = 0; v[3].n = 5;
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

static void fill_block_cross_vao_vertex(ChunkVaoVertex v[4][4], const vec3i &offset) {
    v[0][0].x = 0; v[0][0].y = 0; v[0][0].z = 0; v[0][0].n = 6;
    v[0][1].x = 1; v[0][1].y = 0; v[0][1].z = 1; v[0][1].n = 6;
    v[0][2].x = 1; v[0][2].y = 1; v[0][2].z = 1; v[0][2].n = 6;
    v[0][3].x = 0; v[0][3].y = 1; v[0][3].z = 0; v[0][3].n = 6;

    v[1][0].x = 0; v[1][0].y = 0; v[1][0].z = 1; v[1][0].n = 7;
    v[1][1].x = 1; v[1][1].y = 0; v[1][1].z = 0; v[1][1].n = 7;
    v[1][2].x = 1; v[1][2].y = 1; v[1][2].z = 0; v[1][2].n = 7;
    v[1][3].x = 0; v[1][3].y = 1; v[1][3].z = 1; v[1][3].n = 7;

#if 0
    v[2][1].x = 0; v[2][1].y = 0; v[2][1].z = 0; v[2][1].n = 8;
    v[2][0].x = 1; v[2][0].y = 0; v[2][0].z = 1; v[2][0].n = 8;
    v[2][3].x = 1; v[2][3].y = 1; v[2][3].z = 1; v[2][3].n = 8;
    v[2][2].x = 0; v[2][2].y = 1; v[2][2].z = 0; v[2][2].n = 8;

    v[3][1].x = 0; v[3][1].y = 0; v[3][1].z = 1; v[3][1].n = 9;
    v[3][0].x = 1; v[3][0].y = 0; v[3][0].z = 0; v[3][0].n = 9;
    v[3][3].x = 1; v[3][3].y = 1; v[3][3].z = 0; v[3][3].n = 9;
    v[3][2].x = 0; v[3][2].y = 1; v[3][2].z = 1; v[3][2].n = 9;
#endif

    offset_vertices(v[0], offset);
    offset_vertices(v[1], offset);
    //offset_vertices(v[2], offset);
    //offset_vertices(v[3], offset);
}

static void fill_block_cross_vao_tex_coords(ChunkVaoVertex v[4][4], BlockType type) {
    BlockTextureID texture_id = TEX_SAND;
    switch(type) {
        default: break;

        case BlockType::GRASS: {
            texture_id = BlockTextureID::TEX_GRASS;
        } break;
    }

    fill_tex_coords(v[0]);
    fill_tex_coords(v[1]);
    fill_tex_coords(v[2]);
    fill_tex_coords(v[3]);

    fill_tex_slots(v[0], texture_id);
    fill_tex_slots(v[1], texture_id);
    fill_tex_slots(v[2], texture_id);
    fill_tex_slots(v[3], texture_id);
}

static void push_vao_quad(ChunkVaoVertex quad_vertices[4], ChunkMeshData &data) {
    const int32_t start = data.vertices.size();

    data.vertices.push_back(pack_chunk_vertex(quad_vertices[0]));
    data.vertices.push_back(pack_chunk_vertex(quad_vertices[1]));
    data.vertices.push_back(pack_chunk_vertex(quad_vertices[2]));
    data.vertices.push_back(pack_chunk_vertex(quad_vertices[3]));

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
            /* Pushing 4 quads for crosses because of face culling */
            ChunkVaoVertex v[4][4] = { };
            fill_block_cross_vao_vertex(v, block_rel);
            fill_block_cross_vao_tex_coords(v, type);
            push_vao_quad(v[0], data);
            push_vao_quad(v[1], data);
            push_vao_quad(v[2], data);
            push_vao_quad(v[3], data);
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
                if(flags & HAS_TRANSPARENCY && nb_flags & HAS_TRANSPARENCY && !(nb_flags & IS_NOT_VISIBLE) && block.type == nb_block.type && block.type != BlockType::TREE_LEAVES) {
                    connect_wall[side] = true;
                }
            }
        }

        push_block_vao_data(block.type, gen_data->chunk, block_rel, connect_wall);
    }
}

/* @TODO */
void chunk_mesh_gen_single_block(ChunkMeshData &mesh_data, BlockType type) {
    bool connect_wall[BLOCK_SIDE_COUNT] = {
        false, false, false, 
        false, false, false
    };


    push_block_vao_data(type, mesh_data, { 0, 0, 0 }, connect_wall);

#if 0
    push_block_vao_data(type, mesh_data, { 0, 0, 0 }, connect_wall);
    for(uint32_t index = 0; index < mesh_data.vertices.size(); ++index) {
        ChunkVaoVertex &v = mesh_data.vertices[index];
        v.position -= vec3::make(0.5f);
    }
#endif
}
