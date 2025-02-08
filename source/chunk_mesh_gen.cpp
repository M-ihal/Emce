#include "chunk_mesh_gen.h"
#include "world.h"
#include "world_utils.h"

static vec3i g_ao_offsets[6][4][2] = {
    { { vec3i{ 0, -1, 0 }, vec3i{ 0, 0, -1 } }, { vec3i{ 0, -1, 0 }, vec3i{ 0, 0, +1 } }, { vec3i{ 0, +1, 0 }, vec3i{ 0, 0, +1 } }, { vec3i{ 0, +1, 0 }, vec3i{ 0, 0, -1 } } },
    { { vec3i{ 0, -1, 0 }, vec3i{ 0, 0, +1 } }, { vec3i{ 0, -1, 0 }, vec3i{ 0, 0, -1 } }, { vec3i{ 0, +1, 0 }, vec3i{ 0, 0, -1 } }, { vec3i{ 0, +1, 0 }, vec3i{ 0, 0, +1 } } },
    { { vec3i{ +1, 0, 0 }, vec3i{ 0, -1, 0 } }, { vec3i{ -1, 0, 0 }, vec3i{ 0, -1, 0 } }, { vec3i{ -1, 0, 0 }, vec3i{ 0, +1, 0 } }, { vec3i{ +1, 0, 0 }, vec3i{ 0, +1, 0 } } },
    { { vec3i{ -1, 0, 0 }, vec3i{ 0, -1, 0 } }, { vec3i{ +1, 0, 0 }, vec3i{ 0, -1, 0 } }, { vec3i{ +1, 0, 0 }, vec3i{ 0, +1, 0 } }, { vec3i{ -1, 0, 0 }, vec3i{ 0, +1, 0 } } },
    { { vec3i{ -1, 0, 0 }, vec3i{ 0, 0, -1 } }, { vec3i{ +1, 0, 0 }, vec3i{ 0, 0, -1 } }, { vec3i{ +1, 0, 0 }, vec3i{ 0, 0, +1 } }, { vec3i{ -1, 0, 0 }, vec3i{ 0, 0, +1 } } },
    { { vec3i{ -1, 0, 0 }, vec3i{ 0, 0, +1 } }, { vec3i{ +1, 0, 0 }, vec3i{ 0, 0, +1 } }, { vec3i{ +1, 0, 0 }, vec3i{ 0, 0, -1 } }, { vec3i{ -1, 0, 0 }, vec3i{ 0, 0, -1 } } },
};

inline static void clear_mesh_gen_vertex_data(ChunkMeshGenData *gen_data) {
    gen_data->chunk.vertices.clear();
    gen_data->water.vertices.clear();
    gen_data->chunk.indices.clear();
    gen_data->water.indices.clear();
}

void chunk_mesh_gen_data_init(ChunkMeshGenData *gen_data, World &world, vec2i chunk_coords) {
    clear_mesh_gen_vertex_data(gen_data);
    gen_data->has_been_dropped = false;
    
    Chunk *chunk = world.get_chunk(chunk_coords);
    Chunk *chunk_x_pos = world.get_chunk(chunk_coords + vec2i{ 1, 0 });
    Chunk *chunk_x_neg = world.get_chunk(chunk_coords + vec2i{-1, 0 });
    Chunk *chunk_z_pos = world.get_chunk(chunk_coords + vec2i{ 0, 1 });
    Chunk *chunk_z_neg = world.get_chunk(chunk_coords + vec2i{ 0,-1 });
    Chunk *chunk_x_pos_z_pos = world.get_chunk(chunk_coords + vec2i{ 1, 1 });
    Chunk *chunk_x_neg_z_pos = world.get_chunk(chunk_coords + vec2i{-1, 1 });
    Chunk *chunk_x_neg_z_neg = world.get_chunk(chunk_coords + vec2i{-1,-1 });
    Chunk *chunk_x_pos_z_neg = world.get_chunk(chunk_coords + vec2i{ 1,-1 });

    ASSERT(chunk && chunk_x_pos && chunk_x_neg && chunk_z_pos && chunk_z_neg && chunk_x_pos_z_pos && chunk_x_neg_z_pos && chunk_x_neg_z_neg && chunk_x_pos_z_neg);

    gen_data->chunk_coords = chunk_coords;
    chunk->copy_blocks_from(gen_data->chunk_blocks);

    /* Copy neighbour slices */ {
        for(int32_t x = 0; x < CHUNK_SIZE_X; ++x) {
            for(int32_t y = 0; y < CHUNK_SIZE_Y; ++y) {
                gen_data->chunk_z_pos[y * CHUNK_SIZE_X + x] = chunk_z_pos->get_block({ x, y, 0 });
            }
        }

        for(int32_t x = 0; x < CHUNK_SIZE_X; ++x) {
            for(int32_t y = 0; y < CHUNK_SIZE_Y; ++y) {
                gen_data->chunk_z_neg[y * CHUNK_SIZE_X + x] = chunk_z_neg->get_block({ x, y, CHUNK_SIZE_Z - 1 });
            }
        }

        for(int32_t z = 0; z < CHUNK_SIZE_Z; ++z) {
            for(int32_t y = 0; y < CHUNK_SIZE_Y; ++y) {
                gen_data->chunk_x_pos[y * CHUNK_SIZE_Z + z] = chunk_x_pos->get_block({ 0, y, z });
            }
        }

        for(int32_t z = 0; z < CHUNK_SIZE_Z; ++z) {
            for(int32_t y = 0; y < CHUNK_SIZE_Y; ++y) {
                gen_data->chunk_x_neg[y * CHUNK_SIZE_Z + z] = chunk_x_neg->get_block({ CHUNK_SIZE_X - 1, y, z });
            }
        }
    }

    /* Copy diagonal collumns */ {
        auto copy_collumn = [] (BlockType dest[CHUNK_SIZE_Y], Chunk *src, int32_t x, int32_t z) -> void {
            for(int32_t y = 0; y < CHUNK_SIZE_Y; ++y) {
                dest[y] = src->get_block({ x, y, z });
            }
        };

        copy_collumn(gen_data->chunk_x_pos_z_pos, chunk_x_pos_z_pos, 0, 0);
        copy_collumn(gen_data->chunk_x_neg_z_pos, chunk_x_neg_z_pos, CHUNK_SIZE_X - 1, 0);
        copy_collumn(gen_data->chunk_x_neg_z_neg, chunk_x_neg_z_neg, CHUNK_SIZE_X - 1, CHUNK_SIZE_Z - 1);
        copy_collumn(gen_data->chunk_x_pos_z_neg, chunk_x_pos_z_neg, 0, CHUNK_SIZE_Z - 1);
    }
}

void chunk_mesh_gen_data_free(ChunkMeshGenData *gen_data) {

}

/* Adjacent block by dir from middle block */
static inline const BlockType get_adjacent_block(ChunkMeshGenData *gen_data, const vec3i block_rel, const vec3i offset) {
    // offset must be -1 to 1
    ASSERT(offset.x >= -1 && offset.y <= 1 && offset.y >= -1 && offset.y <= 1 && offset.z >= -1 && offset.z <= 1);
    // block_rel mut be inside chunk
    ASSERT(is_inside_chunk(block_rel));

    const vec3i adj_rel = block_rel + offset;
    const bool in_chunk = is_inside_chunk(adj_rel);
    if(in_chunk) {
        return gen_data->chunk_blocks[get_block_array_index(adj_rel)];
    } else {
        if(adj_rel.y < 0 || adj_rel.y >= CHUNK_SIZE_Y) {
            /* Is above or below chunk -> Act like it's AIR */
            return BlockType::AIR;
        } else if(adj_rel.x == -1 && adj_rel.z == -1) {
            /* Is on the -x -z corner */
            return gen_data->chunk_x_neg_z_neg[adj_rel.y];
        } else if(adj_rel.x == CHUNK_SIZE_X && adj_rel.z == -1) {
            /* Is on the +x -z corner */
            return gen_data->chunk_x_pos_z_neg[adj_rel.y];
        } else if(adj_rel.x == CHUNK_SIZE_X && adj_rel.z == CHUNK_SIZE_Z) {
            /* Is on the +x +z corner */
            return gen_data->chunk_x_pos_z_pos[adj_rel.y];
        } else if(adj_rel.x == -1 && adj_rel.z == CHUNK_SIZE_Z) { 
            /* Is on the -x +z corner */
            return gen_data->chunk_x_neg_z_pos[adj_rel.y];
        } else {
            /* Is on the side */
            if(adj_rel.x == -1) {
                return gen_data->chunk_x_neg[adj_rel.y * CHUNK_SIZE_Z + adj_rel.z];
            } else if(adj_rel.x == CHUNK_SIZE_X) {
                return gen_data->chunk_x_pos[adj_rel.y * CHUNK_SIZE_Z + adj_rel.z];
            } else if(adj_rel.z == -1) {
                return gen_data->chunk_z_neg[adj_rel.y * CHUNK_SIZE_X + adj_rel.x];
            } else {
                return gen_data->chunk_z_pos[adj_rel.y * CHUNK_SIZE_X + adj_rel.x];
            }
        }
    }

    INVALID_CODE_PATH;
    return BlockType::AIR;
}

static inline constexpr bool should_generate_side_mesh(BlockType block, BlockType other) {
    const uint32_t block_flags = get_block_type_flags(block);
    const uint32_t other_flags = get_block_type_flags(other);

    bool do_not_generate_side = block_flags & IS_SOLID && other_flags & IS_SOLID;
    return !do_not_generate_side;
}

static inline void offset_vertex_positions(ChunkVaoVertex v[4], vec3i offset) {
    v[0].x += offset.x; v[0].y += offset.y; v[0].z += offset.z;
    v[1].x += offset.x; v[1].y += offset.y; v[1].z += offset.z;
    v[2].x += offset.x; v[2].y += offset.y; v[2].z += offset.z;
    v[3].x += offset.x; v[3].y += offset.y; v[3].z += offset.z;
}

static inline constexpr void set_cube_vertex_positions(ChunkVaoVertex v[4], vec3i block_rel, BlockSide side) {
    switch(side) {
        default: INVALID_CODE_PATH; return;

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

    offset_vertex_positions(v, block_rel);
}

static inline constexpr void set_cube_vertex_tex_coords(ChunkVaoVertex v[4]) {
    v[0].tc = 0;
    v[1].tc = 1;
    v[2].tc = 2;
    v[3].tc = 3;
}

static inline constexpr void set_cube_vertex_tex_slots(ChunkVaoVertex v[4], const BlockTexture tex_slot) {
    const uint8_t tex_slot_ = (const uint8_t)tex_slot;
    v[0].ts = tex_slot_;
    v[1].ts = tex_slot_;
    v[2].ts = tex_slot_;
    v[3].ts = tex_slot_;
}

static inline constexpr bool block_casts_ambient_occlusion(const BlockType block) {
    return get_block_type_shape(block) != BlockShape::CROSS && !(get_block_type_flags(block) & NO_AO_CAST);
}

static inline constexpr void set_cube_vertex_ao_value(ChunkVaoVertex v[4], ChunkMeshGenData *gen_data, vec3i block_rel, const BlockSide side, const vec3i side_dir) {
    /* Check if neighbouring to side block is solid */ {
        BlockType con_block = get_adjacent_block(gen_data, block_rel, side_dir);
        if(block_casts_ambient_occlusion(con_block)) {
            v[0].ao = 0; // 1?
            v[1].ao = 0;
            v[2].ao = 0;
            v[3].ao = 0;
        }
    }

    const uint8_t side_index = (uint8_t)side;

    /* For every vertice, calculate AO value */
    for(int32_t index = 0; index < 4; ++index) {
        const BlockType block_0 = get_adjacent_block(gen_data, block_rel, g_ao_offsets[side_index][index][0] + side_dir);
        const BlockType block_1 = get_adjacent_block(gen_data, block_rel, g_ao_offsets[side_index][index][1] + side_dir);

        const bool block_0_casts = block_casts_ambient_occlusion(block_0);
        const bool block_1_casts = block_casts_ambient_occlusion(block_1);

        if(block_0_casts && block_1_casts) {
            v[index].ao = 0;
        } else {
            const BlockType block_corner = get_adjacent_block(gen_data, block_rel, g_ao_offsets[side_index][index][0] + g_ao_offsets[side_index][index][1] + side_dir);
            const bool block_corner_casts = block_casts_ambient_occlusion(block_corner);

            v[index].ao = 3 - ((int32_t)block_0_casts + (int32_t)block_1_casts + (int32_t)block_corner_casts);
        }
    }
}

static void gen_chunk_block_mesh(ChunkMeshGenData *gen_data, const vec3i block_rel, BlockType block) {
    const uint32_t block_flags = get_block_type_flags(block);

    const bool generates_mesh = !(block_flags & IS_NOT_VISIBLE);
    if(!generates_mesh) {
        return;
    }

    const BlockShape shape = get_block_type_shape(block);
    switch(shape) {
        default: ASSERT(false); return;

        case BlockShape::NONE: return;

        case BlockShape::CUBE: {
            for(int32_t side_index = 0; side_index < 6; ++side_index) {
                const BlockSide side = (BlockSide)side_index;
                const vec3i side_dir = get_block_side_normal(side);

                ChunkVaoVertex v[4] = { };

                const bool generate_side = should_generate_side_mesh(block, get_adjacent_block(gen_data, block_rel, side_dir));
                if(!generate_side) {
                    continue;
                }

                const BlockTexture tex_slot = get_block_cube_texture(block, side);

                set_cube_vertex_positions(v, block_rel, side);
                set_cube_vertex_tex_coords(v);
                set_cube_vertex_tex_slots(v, tex_slot);
                set_cube_vertex_ao_value(v, gen_data, block_rel, side, side_dir);

                ChunkMeshData &data = gen_data->chunk;

                const int32_t start = data.vertices.size();

                data.vertices.push_back(pack_chunk_vertex(v[0]));
                data.vertices.push_back(pack_chunk_vertex(v[1]));
                data.vertices.push_back(pack_chunk_vertex(v[2]));
                data.vertices.push_back(pack_chunk_vertex(v[3]));

                if(v[0].ao + v[2].ao < v[1].ao + v[3].ao) {
                    data.indices.push_back(start + 1);
                    data.indices.push_back(start + 2);
                    data.indices.push_back(start + 3);
                    data.indices.push_back(start + 3);
                    data.indices.push_back(start + 0);
                    data.indices.push_back(start + 1);
                } else {
                    data.indices.push_back(start + 0);
                    data.indices.push_back(start + 1);
                    data.indices.push_back(start + 2);
                    data.indices.push_back(start + 2);
                    data.indices.push_back(start + 3);
                    data.indices.push_back(start + 0);
                }
            }
        } break;

        case BlockShape::CROSS: {

        } break;
    }
}

static void gen_water_block_mesh(ChunkMeshGenData *gen_data, const vec3i block_rel, BlockType adjacent[3][3][3]) {

}

void chunk_mesh_gen(ChunkMeshGenData *gen_data) {
    for_every_block(x, y, z) {
        const vec3i block_rel = { x, y, z };
        const BlockType block = gen_data->chunk_blocks[get_block_array_index(block_rel)];
        const uint32_t block_flags = get_block_type_flags(block);
        if(block == BlockType::AIR && block_flags & IS_NOT_VISIBLE) {
            continue;
        }

        if(block == BlockType::WATER) {
            // gen_water_block_mesh(gen_data, block_rel, adjacent);
        } else {
            gen_chunk_block_mesh(gen_data, block_rel, block);
        }
    }

   // PRINT_INT(gen_data->chunk.vertices.size());
}

void chunk_mesh_gen_single_block(ChunkMeshData &mesh_data, BlockType type) {

}

#if 0

static inline uint32_t get_block_array_index(const vec3i &block_rel) {
    return block_rel.x * (CHUNK_SIZE_Y * CHUNK_SIZE_Z) + block_rel.y * CHUNK_SIZE_Z + block_rel.z;
}

static inline BlockType get_block(ChunkMeshGenData *gen_data, const vec3i &block_rel) {
    BlockType block;
    if(is_inside_chunk(block_rel)) {
        block = gen_data->chunk_blocks[get_block_array_index(block_rel)];
    } else {
        INVALID_CODE_PATH;
    }
    return block;
}

/* Get block from the border of any neighbour */
static inline BlockType get_block_bordering(ChunkMeshGenData *gen_data, vec3i block_rel) {

    /* If outside Y boundaries -> Assume block is AIR */
    if(block_rel.y < 0 || block_rel.y >= CHUNK_SIZE_Y) {
        return BlockType::AIR;
    }

    vec2i chunk_offset = { };
    if(block_rel.x < 0) {
        block_rel.x = CHUNK_SIZE_X - 1;
        chunk_offset.x = -1;
    }
    if(block_rel.x >= CHUNK_SIZE_X) {
        block_rel.x = 0;
        chunk_offset.x = 1;
    }
    if(block_rel.z < 0) {
        block_rel.z = CHUNK_SIZE_Z - 1;
        chunk_offset.y = -1;
    }
    if(block_rel.z >= CHUNK_SIZE_Z) {
        block_rel.z = 0;
        chunk_offset.y = 1;
    }

    if(chunk_offset == vec2i{ -1, 0 }) {
        return gen_data->chunk_x_neg[get_block_array_index(block_rel)];
    } else if(chunk_offset == vec2i{ +1, 0 }) {
        return gen_data->chunk_x_pos[get_block_array_index(block_rel)];
    } else if(chunk_offset == vec2i{ 0, -1 }) {
        return gen_data->chunk_z_neg[get_block_array_index(block_rel)];
    } else if(chunk_offset == vec2i{ 0, +1 }) {
        return gen_data->chunk_z_pos[get_block_array_index(block_rel)];
    } else if(chunk_offset == vec2i{ -1, -1 }) {
        return gen_data->chunk_x_neg_z_neg[get_block_array_index(block_rel)];
    } else if(chunk_offset == vec2i{ +1, +1 }) {
        return gen_data->chunk_x_pos_z_pos[get_block_array_index(block_rel)];
    } else if(chunk_offset == vec2i{ -1, +1 }) {
        return gen_data->chunk_x_neg_z_pos[get_block_array_index(block_rel)];
    } else if(chunk_offset == vec2i{ +1, -1 }) {
        return gen_data->chunk_x_pos_z_neg[get_block_array_index(block_rel)];
    } else {
        INVALID_CODE_PATH;
        return BlockType::AIR;
    }
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

static vec3i g_ao_offsets[6][4][2] = {
    { { vec3i{ 0, -1, 0 }, vec3i{ 0, 0, -1 } }, { vec3i{ 0, -1, 0 }, vec3i{ 0, 0, +1 } }, { vec3i{ 0, +1, 0 }, vec3i{ 0, 0, +1 } }, { vec3i{ 0, +1, 0 }, vec3i{ 0, 0, -1 } } },
    { { vec3i{ 0, -1, 0 }, vec3i{ 0, 0, +1 } }, { vec3i{ 0, -1, 0 }, vec3i{ 0, 0, -1 } }, { vec3i{ 0, +1, 0 }, vec3i{ 0, 0, -1 } }, { vec3i{ 0, +1, 0 }, vec3i{ 0, 0, +1 } } },
    { { vec3i{ +1, 0, 0 }, vec3i{ 0, -1, 0 } }, { vec3i{ -1, 0, 0 }, vec3i{ 0, -1, 0 } }, { vec3i{ -1, 0, 0 }, vec3i{ 0, +1, 0 } }, { vec3i{ +1, 0, 0 }, vec3i{ 0, +1, 0 } } },
    { { vec3i{ -1, 0, 0 }, vec3i{ 0, -1, 0 } }, { vec3i{ +1, 0, 0 }, vec3i{ 0, -1, 0 } }, { vec3i{ +1, 0, 0 }, vec3i{ 0, +1, 0 } }, { vec3i{ -1, 0, 0 }, vec3i{ 0, +1, 0 } } },
    { { vec3i{ -1, 0, 0 }, vec3i{ 0, 0, -1 } }, { vec3i{ +1, 0, 0 }, vec3i{ 0, 0, -1 } }, { vec3i{ +1, 0, 0 }, vec3i{ 0, 0, +1 } }, { vec3i{ -1, 0, 0 }, vec3i{ 0, 0, +1 } } },
    { { vec3i{ -1, 0, 0 }, vec3i{ 0, 0, +1 } }, { vec3i{ +1, 0, 0 }, vec3i{ 0, 0, +1 } }, { vec3i{ +1, 0, 0 }, vec3i{ 0, 0, -1 } }, { vec3i{ -1, 0, 0 }, vec3i{ 0, 0, -1 } } },
};

inline constexpr bool block_casts_ao(BlockType type) {
    BlockShape shape = get_block_type_shape(type);
    uint32_t flags = get_block_type_flags(type);
    return shape != BlockShape::CROSS && !(flags & NO_AO_CAST);
}

void fill_block_cube_vao_ao(ChunkMeshGenData *gen, ChunkVaoVertex v[4], BlockSide side, const vec3i &block_rel, bool connect_wall[6]) {
    const int32_t side_id = (int32_t)side;
    vec3i normal = get_block_side_normal(side);

    vec3i _block = block_rel + normal;
    BlockType _BLOCK;
    if(is_inside_chunk(_block)) {
        _BLOCK = get_block(gen, _block);
    } else {
        _BLOCK = get_block_bordering(gen, _block);
    }
    if(_BLOCK != BlockType::AIR && get_block_type_shape(_BLOCK) != BlockShape::CROSS && !(get_block_type_flags(_BLOCK) & NO_AO_CAST)) {
        v[0].ao = 1;
        v[1].ao = 1;
        v[2].ao = 1;
        v[3].ao = 1;
        return;
    }

    for(int32_t vert = 0; vert < 4; ++vert) {
        vec3i block_0 = block_rel + g_ao_offsets[side_id][vert][0] + normal;
        vec3i block_1 = block_rel + g_ao_offsets[side_id][vert][1] + normal;

        BlockType block__0;
        if(is_inside_chunk(block_0)) {
            block__0 = get_block(gen, block_0);
        } else {
            block__0 = get_block_bordering(gen, block_0);
        }

        BlockType block__1;
        if(is_inside_chunk(block_1)) {
            block__1 = get_block(gen, block_1);
        } else {
            block__1 = get_block_bordering(gen, block_1);
        }

        bool side_0 = block_casts_ao(block__0);
        bool side_1 = block_casts_ao(block__1);

        if(side_0 && side_1) {
            v[vert].ao = 0;
        } else {
            vec3i block_corner = block_rel + g_ao_offsets[side_id][vert][0] + g_ao_offsets[side_id][vert][1] + normal;

            BlockType block__corner;
            if(is_inside_chunk(block_corner)) {
                block__corner = get_block(gen, block_corner);
            } else {
                block__corner = get_block_bordering(gen, block_corner);
            }

            bool corner = block_casts_ao(block__corner);

            v[vert].ao = 3 - ((int32_t)side_0 + (int32_t)side_1 + (int32_t)corner);
        }
    }
}

static void fill_block_cube_vao_tex_coords(ChunkVaoVertex v[4], BlockType type, BlockSide side) {
    BlockTexture texture = get_block_cube_texture(type, side);
    fill_tex_coords(v);
    fill_tex_slots(v, (uint8_t)texture);
}

static void fill_block_cross_vao_vertex(ChunkVaoVertex v[4][4], const vec3i &offset) {
    v[0][0].x = 0; v[0][0].y = 0; v[0][0].z = 0; v[0][0].n = 7; v[0][0].ao = 3;
    v[0][1].x = 1; v[0][1].y = 0; v[0][1].z = 1; v[0][1].n = 7; v[0][1].ao = 3;
    v[0][2].x = 1; v[0][2].y = 1; v[0][2].z = 1; v[0][2].n = 7; v[0][2].ao = 3;
    v[0][3].x = 0; v[0][3].y = 1; v[0][3].z = 0; v[0][3].n = 7; v[0][3].ao = 3;

    v[1][0].x = 0; v[1][0].y = 0; v[1][0].z = 1; v[1][0].n = 6; v[1][0].ao = 3;
    v[1][1].x = 1; v[1][1].y = 0; v[1][1].z = 0; v[1][1].n = 6; v[1][1].ao = 3;
    v[1][2].x = 1; v[1][2].y = 1; v[1][2].z = 0; v[1][2].n = 6; v[1][2].ao = 3;
    v[1][3].x = 0; v[1][3].y = 1; v[1][3].z = 1; v[1][3].n = 6; v[1][3].ao = 3;

    v[2][1].x = 0; v[2][1].y = 0; v[2][1].z = 0; v[2][1].n = 9; v[2][1].ao = 3;
    v[2][0].x = 1; v[2][0].y = 0; v[2][0].z = 1; v[2][0].n = 9; v[2][0].ao = 3;
    v[2][3].x = 1; v[2][3].y = 1; v[2][3].z = 1; v[2][3].n = 9; v[2][3].ao = 3;
    v[2][2].x = 0; v[2][2].y = 1; v[2][2].z = 0; v[2][2].n = 9; v[2][2].ao = 3;

    v[3][1].x = 0; v[3][1].y = 0; v[3][1].z = 1; v[3][1].n = 8; v[3][1].ao = 3;
    v[3][0].x = 1; v[3][0].y = 0; v[3][0].z = 0; v[3][0].n = 8; v[3][0].ao = 3;
    v[3][3].x = 1; v[3][3].y = 1; v[3][3].z = 0; v[3][3].n = 8; v[3][3].ao = 3;
    v[3][2].x = 0; v[3][2].y = 1; v[3][2].z = 1; v[3][2].n = 8; v[3][2].ao = 3;

    offset_vertices(v[0], offset);
    offset_vertices(v[1], offset);
    offset_vertices(v[2], offset);
    offset_vertices(v[3], offset);
}

static void fill_block_cross_vao_tex_coords(ChunkVaoVertex v[4][4], BlockType type) {
    BlockTexture texture = get_block_cross_texture(type);

    fill_tex_coords(v[0]);
    fill_tex_coords(v[1]);
    fill_tex_coords(v[2]);
    fill_tex_coords(v[3]);

    fill_tex_slots(v[0], (int32_t)texture);
    fill_tex_slots(v[1], (int32_t)texture);
    fill_tex_slots(v[2], (int32_t)texture);
    fill_tex_slots(v[3], (int32_t)texture);
}


static void push_vao_quad(ChunkVaoVertex quad_vertices[4], ChunkMeshData &data) {
    const int32_t start = data.vertices.size();

    data.vertices.push_back(pack_chunk_vertex(quad_vertices[0]));
    data.vertices.push_back(pack_chunk_vertex(quad_vertices[1]));
    data.vertices.push_back(pack_chunk_vertex(quad_vertices[2]));
    data.vertices.push_back(pack_chunk_vertex(quad_vertices[3]));

    if(quad_vertices[0].ao + quad_vertices[2].ao < quad_vertices[1].ao + quad_vertices[3].ao) {
        data.indices.push_back(start + 1);
        data.indices.push_back(start + 2);
        data.indices.push_back(start + 3);
        data.indices.push_back(start + 3);
        data.indices.push_back(start + 0);
        data.indices.push_back(start + 1);
    } else {
        data.indices.push_back(start + 0);
        data.indices.push_back(start + 1);
        data.indices.push_back(start + 2);
        data.indices.push_back(start + 2);
        data.indices.push_back(start + 3);
        data.indices.push_back(start + 0);
    }
}

static void push_water_vao_data(ChunkMeshGenData *gen_data, ChunkMeshData &data, const vec3i &block_rel, bool connect_wall[6]) {
    for(int32_t side = 0; side < 6; ++side) {
        ChunkVaoVertex v[4] = { };

        if(!connect_wall[side]) {
            fill_block_cube_vao_vertex(v, (BlockSide)side, block_rel);
            fill_tex_coords(v);
            fill_tex_slots(v, 0); /* Tex slot in water means water frame */
            if(gen_data) {
                fill_block_cube_vao_ao(gen_data, v, (BlockSide)side, block_rel, connect_wall);
            } else {
                v[0].ao = 3;
                v[1].ao = 3;
                v[2].ao = 3;
                v[3].ao = 3;
            }
            push_vao_quad(v, data);

#if 0
            /* If facing up, duplicate quad from other side */
            if(side == (int32_t)BlockSide::Y_POS) {
                ChunkVaoVertex temp = v[0];
                v[0] = v[1];
                v[1] = temp;

                temp = v[2];
                v[2] = v[3];
                v[3] = temp;
                push_vao_quad(v, data);
            }
#endif
        }
    }
}


static void push_block_vao_data(ChunkMeshGenData *gen_data, BlockType type, ChunkMeshData &data, const vec3i &block_rel, bool connect_wall[6]) {

    BlockShape shape = get_block_type_shape(type);
    switch(shape) {
        default: {
            return;
        } break;

        case BlockShape::CUBE: {
            for(int32_t side = 0; side < 6; ++side) {
                ChunkVaoVertex v[4] = { };
                if(!connect_wall[side]) {
                    fill_block_cube_vao_vertex(v, (BlockSide)side, block_rel);
                    fill_block_cube_vao_tex_coords(v, type, (BlockSide)side);
                    if(gen_data && !(get_block_type_flags(type) & NO_AO)) {
                        fill_block_cube_vao_ao(gen_data, v, (BlockSide)side, block_rel, connect_wall);
                    } else {
                        v[0].ao = 3;
                        v[1].ao = 3;
                        v[2].ao = 3;
                        v[3].ao = 3;
                    }
                    push_vao_quad(v, data);
                }
            }
        } break;

        case BlockShape::CROSS: {
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

void chunk_mesh_gen_data_init(ChunkMeshGenData *gen_data, World &world, vec2i chunk_coords) {
    gen_data->chunk.indices.clear();
    gen_data->chunk.vertices.clear();
    gen_data->water.indices.clear();
    gen_data->water.vertices.clear();

    Chunk *chunk = world.get_chunk(chunk_coords);
    Chunk *chunk_z_neg = world.get_chunk(chunk_coords + vec2i{ 0, -1 });
    Chunk *chunk_z_pos = world.get_chunk(chunk_coords + vec2i{ 0,  1 });
    Chunk *chunk_x_neg = world.get_chunk(chunk_coords + vec2i{-1,  0 });
    Chunk *chunk_x_pos = world.get_chunk(chunk_coords + vec2i{ 1,  0 });
    Chunk *chunk_x_neg_z_neg = world.get_chunk(chunk_coords + vec2i{-1, -1 });
    Chunk *chunk_x_pos_z_pos = world.get_chunk(chunk_coords + vec2i{ 1,  1 });
    Chunk *chunk_x_neg_z_pos = world.get_chunk(chunk_coords + vec2i{-1,  1 });
    Chunk *chunk_x_pos_z_neg = world.get_chunk(chunk_coords + vec2i{ 1, -1 });

    /* Meshing should only appear if neighbouring blocks exist */
    ASSERT(chunk && chunk_x_neg && chunk_x_pos && chunk_z_neg && chunk_z_pos && chunk_x_neg_z_neg && chunk_x_pos_z_pos && chunk_x_neg_z_pos && chunk_x_pos_z_neg);

    const size_t chunk_blocks_size = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * sizeof(BlockType);

    /* Copy the main chunk's blocks */
    gen_data->chunk_coords = chunk_coords;
    chunk->copy_blocks_from(gen_data->chunk_blocks);

    /* Copy the neighbouring chunks' blocks */
    chunk_x_neg->copy_blocks_from(gen_data->chunk_x_neg);
    chunk_x_pos->copy_blocks_from(gen_data->chunk_x_pos);
    chunk_z_neg->copy_blocks_from(gen_data->chunk_z_neg);
    chunk_z_pos->copy_blocks_from(gen_data->chunk_z_pos);
    chunk_x_neg_z_neg->copy_blocks_from(gen_data->chunk_x_neg_z_neg);
    chunk_x_pos_z_pos->copy_blocks_from(gen_data->chunk_x_pos_z_pos);
    chunk_x_neg_z_pos->copy_blocks_from(gen_data->chunk_x_neg_z_pos);
    chunk_x_pos_z_neg->copy_blocks_from(gen_data->chunk_x_pos_z_neg);

}

void chunk_mesh_gen_data_free(ChunkMeshGenData *gen_data) {
    gen_data->chunk.vertices.clear();
    gen_data->chunk.indices.clear();
    gen_data->water.vertices.clear();
    gen_data->water.indices.clear();
    gen_data->chunk.vertices.shrink_to_fit();
    gen_data->chunk.indices.shrink_to_fit();
    gen_data->water.vertices.shrink_to_fit();
    gen_data->water.indices.shrink_to_fit();
}

void chunk_mesh_gen(ChunkMeshGenData *gen_data) {
    for_every_block(x, y, z) {
        vec3i block_rel = { x, y, z };
        BlockType block = get_block(gen_data, block_rel);

        /* Water goes to different vao */
        if(block == BlockType::WATER) {
            bool connect_wall[8] = { };
            for(int32_t side = 0; side < 6; ++side) {
                vec3i nb_block_rel = block_rel + get_block_side_normal((BlockSide)side);

                BlockType nb_block = BlockType::AIR;
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

                if(nb_block!= BlockType::AIR) {
                    uint32_t nb_flags = get_block_type_flags(nb_block);
                    connect_wall[side] = !(nb_flags & IS_NOT_VISIBLE) && !(nb_flags & HAS_TRANSPARENCY);


                    if(nb_block == BlockType::WATER) {
                        connect_wall[side] = true;
                    }
                }
            }

            push_water_vao_data(gen_data, gen_data->water, block_rel, connect_wall);
            continue;
        }

        const uint32_t flags = get_block_type_flags(block);
        if(flags & IS_NOT_VISIBLE) {
            continue;
        }

        bool connect_wall[6] = { };

        for(int32_t side = 0; side < 6; ++side) {
            vec3i nb_block_rel = block_rel + get_block_side_normal((BlockSide)side);

            BlockType nb_block = BlockType::AIR;
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

            uint32_t nb_flags = get_block_type_flags(nb_block);


            if(nb_flags & IS_NOT_VISIBLE) {
                connect_wall[side] = false;
            } else if(nb_flags & HAS_TRANSPARENCY) {
                if(flags & CONNECTS_WITH_SAME_TYPE && block == nb_block) {
                    connect_wall[side] = true;
                } else {
                    connect_wall[side] = false;
                }
            } else {
                connect_wall[side] = true;
            }
        }

        push_block_vao_data(gen_data, block, gen_data->chunk, block_rel, connect_wall);
    }
}

void chunk_mesh_gen_single_block(ChunkMeshData &mesh_data, BlockType type) {
    bool connect_wall[6] = {
        false, false, false, 
        false, false, false
    };

    push_block_vao_data(NULL, type, mesh_data, { 0, 0, 0 }, connect_wall);
}

#endif
