#include "chunk_mesh_gen.h"
#include "world.h"
#include "world_utils.h"

static constexpr uint32_t AO_MAX = 3;

static constexpr vec3i g_ao_offsets[6][4][2] = {
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

    gen_data->chunk_mesh_build_id = chunk->get_mesh_build_counter_next();

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
    // block_rel must be inside chunk
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

    bool generate_side = true;

    if(other_flags & IS_NOT_VISIBLE) {
        generate_side = true;
    } else if(other_flags & HAS_TRANSPARENCY) {
        if(other_flags & CONNECTS_WITH_SAME_TYPE && block == other) {
            generate_side = false;
        } else {
            generate_side = true;
        }
    } else {
        generate_side = false;
    }

    return generate_side;
}

static inline constexpr bool should_generate_water_side_mesh(ChunkMeshGenData *gen_data, const vec3i block_rel, const BlockSide side) {
    const vec3i side_dir = get_block_side_normal(side);
    const BlockType adj_block = get_adjacent_block(gen_data, block_rel, side_dir);
 
    bool generate_side = true;

    if(adj_block == BlockType::WATER) {
        generate_side = false;
    } else if(side == BlockSide::Y_POS) {
        generate_side = true;
    } else {
        const uint32_t adj_flags = get_block_type_flags(adj_block);
        if(adj_flags & HAS_TRANSPARENCY || adj_flags & IS_NOT_VISIBLE) {
            generate_side = true;
        } else {
            generate_side = false; /* Water animation bug ... */
        }
    }

    return generate_side;
}

static inline void offset_vertex_positions(ChunkVaoVertex v[4], vec3i offset) {
    v[0].x += offset.x; v[0].y += offset.y; v[0].z += offset.z;
    v[1].x += offset.x; v[1].y += offset.y; v[1].z += offset.z;
    v[2].x += offset.x; v[2].y += offset.y; v[2].z += offset.z;
    v[3].x += offset.x; v[3].y += offset.y; v[3].z += offset.z;
}

static inline constexpr void set_vertex_positions_cube(ChunkVaoVertex v[4], vec3i block_rel, BlockSide side) {
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

static inline constexpr void set_vertex_positions_cross(ChunkVaoVertex v[4][4], const vec3i block_rel) {
    constexpr uint32_t AO_MAX = 3;

    v[0][0].x = 0; v[0][0].y = 0; v[0][0].z = 0; v[0][0].n = 7;
    v[0][1].x = 1; v[0][1].y = 0; v[0][1].z = 1; v[0][1].n = 7;
    v[0][2].x = 1; v[0][2].y = 1; v[0][2].z = 1; v[0][2].n = 7;
    v[0][3].x = 0; v[0][3].y = 1; v[0][3].z = 0; v[0][3].n = 7;

    v[1][1].x = 1; v[1][1].y = 0; v[1][1].z = 0; v[1][1].n = 6;
    v[1][0].x = 0; v[1][0].y = 0; v[1][0].z = 1; v[1][0].n = 6;
    v[1][2].x = 1; v[1][2].y = 1; v[1][2].z = 0; v[1][2].n = 6;
    v[1][3].x = 0; v[1][3].y = 1; v[1][3].z = 1; v[1][3].n = 6;

    v[2][1].x = 0; v[2][1].y = 0; v[2][1].z = 0; v[2][1].n = 9;
    v[2][0].x = 1; v[2][0].y = 0; v[2][0].z = 1; v[2][0].n = 9;
    v[2][3].x = 1; v[2][3].y = 1; v[2][3].z = 1; v[2][3].n = 9;
    v[2][2].x = 0; v[2][2].y = 1; v[2][2].z = 0; v[2][2].n = 9;

    v[3][1].x = 0; v[3][1].y = 0; v[3][1].z = 1; v[3][1].n = 8;
    v[3][0].x = 1; v[3][0].y = 0; v[3][0].z = 0; v[3][0].n = 8;
    v[3][3].x = 1; v[3][3].y = 1; v[3][3].z = 0; v[3][3].n = 8;
    v[3][2].x = 0; v[3][2].y = 1; v[3][2].z = 1; v[3][2].n = 8;

    offset_vertex_positions(v[1], block_rel);
    offset_vertex_positions(v[0], block_rel);
    offset_vertex_positions(v[2], block_rel);
    offset_vertex_positions(v[3], block_rel);
}

static inline constexpr void set_vertex_tex_coords(ChunkVaoVertex v[4]) {
    v[0].tc = 0;
    v[1].tc = 1;
    v[2].tc = 2;
    v[3].tc = 3;
}

static inline constexpr void set_vertex_tex_slots(ChunkVaoVertex v[4], const BlockTexture tex_slot) {
    const uint8_t tex_slot_ = (const uint8_t)tex_slot;
    v[0].ts = tex_slot_;
    v[1].ts = tex_slot_;
    v[2].ts = tex_slot_;
    v[3].ts = tex_slot_;
}

static inline constexpr void set_vertex_ao_max(ChunkVaoVertex v[4]) {
    v[0].ao = AO_MAX;
    v[1].ao = AO_MAX;
    v[2].ao = AO_MAX;
    v[3].ao = AO_MAX;
}

static inline constexpr bool block_casts_ambient_occlusion(const BlockType block) {
    return get_block_type_shape(block) != BlockShape::CROSS && !(get_block_type_flags(block) & NO_AO_CAST);
}

static inline constexpr void set_vertex_ao_value_cube(ChunkVaoVertex v[4], ChunkMeshGenData *gen_data, vec3i block_rel, const BlockSide side, const vec3i side_dir) {
    /* Check if neighbouring to side block is solid */ {
        const BlockType con_block = get_adjacent_block(gen_data, block_rel, side_dir);
        if(block_casts_ambient_occlusion(con_block)) {
            v[0].ao = 1; // 1?
            v[1].ao = 1;
            v[2].ao = 1;
            v[3].ao = 1;
            return;
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

static inline constexpr bool quad_should_be_cw_because_of_ao(ChunkVaoVertex v[4]) {
    return (v[0].ao + v[2].ao) < (v[1].ao + v[3].ao);
}

static inline void push_vertex_quad_ccw(ChunkMeshData &data, ChunkVaoVertex v[4]) {
    const int32_t start = data.vertices.size();

    data.vertices.push_back(pack_chunk_vertex(v[0]));
    data.vertices.push_back(pack_chunk_vertex(v[1]));
    data.vertices.push_back(pack_chunk_vertex(v[2]));
    data.vertices.push_back(pack_chunk_vertex(v[3]));

    data.indices.push_back(start + 0);
    data.indices.push_back(start + 1);
    data.indices.push_back(start + 2);
    data.indices.push_back(start + 2);
    data.indices.push_back(start + 3);
    data.indices.push_back(start + 0);
}

static inline void push_vertex_quad_cw(ChunkMeshData &data, ChunkVaoVertex v[4]) {
    const int32_t start = data.vertices.size();

    data.vertices.push_back(pack_chunk_vertex(v[0]));
    data.vertices.push_back(pack_chunk_vertex(v[1]));
    data.vertices.push_back(pack_chunk_vertex(v[2]));
    data.vertices.push_back(pack_chunk_vertex(v[3]));

    data.indices.push_back(start + 1);
    data.indices.push_back(start + 2);
    data.indices.push_back(start + 3);
    data.indices.push_back(start + 3);
    data.indices.push_back(start + 0);
    data.indices.push_back(start + 1);
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

        case BlockShape::CUBE: {
            for(int32_t side_index = 0; side_index < 6; ++side_index) {
                const BlockSide side = (BlockSide)side_index;
                const vec3i side_dir = get_block_side_normal(side);

                const bool generate_side = should_generate_side_mesh(block, get_adjacent_block(gen_data, block_rel, side_dir));
                if(!generate_side) {
                    continue;
                }

                ChunkVaoVertex v[4];
                const BlockTexture tex_slot = get_block_cube_texture(block, side);

                set_vertex_positions_cube(v, block_rel, side);
                set_vertex_ao_value_cube(v, gen_data, block_rel, side, side_dir);
                set_vertex_tex_coords(v);
                set_vertex_tex_slots(v, tex_slot);

                if(quad_should_be_cw_because_of_ao(v)) {
                    push_vertex_quad_cw(gen_data->chunk, v);
                } else {
                    push_vertex_quad_ccw(gen_data->chunk, v);
                }
            }
        } break;

        case BlockShape::CROSS: {
            ChunkVaoVertex v[4][4];
            const BlockTexture tex_slot = get_block_cross_texture(block);

            /* Set vertex positions */
            set_vertex_positions_cross(v, block_rel);

            /* Set common tex values and push quads */
            for(int32_t index = 0; index < 4; ++index) {
                set_vertex_ao_max(v[index]);
                set_vertex_tex_coords(v[index]);
                set_vertex_tex_slots(v[index], tex_slot);
                push_vertex_quad_cw(gen_data->chunk, v[index]);
            }
        } break;
    }
}

static void gen_water_block_mesh(ChunkMeshGenData *gen_data, const vec3i block_rel) {
    for(int32_t side_index = 0; side_index < 6; ++side_index) {
        const BlockSide side = (BlockSide)side_index;
    
        const bool generate_side = should_generate_water_side_mesh(gen_data, block_rel, side);
        if(!generate_side) {
            continue;
        }

        ChunkVaoVertex v[4];
        set_vertex_positions_cube(v, block_rel, side);
        set_vertex_tex_coords(v);
        set_vertex_tex_slots(v, (BlockTexture)0);
        set_vertex_ao_max(v);

        push_vertex_quad_cw(gen_data->water, v);
    }
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
            gen_water_block_mesh(gen_data, block_rel);
        } else {
            gen_chunk_block_mesh(gen_data, block_rel, block);
        }
    }
}

void chunk_mesh_gen_single_block(ChunkMeshData &mesh_data, BlockType block) {
    const vec3i block_offset = { 0, 0, 0 };
    const BlockShape shape = get_block_type_shape(block);

    switch(shape) {
        default: return;

        case BlockShape::CUBE: {
            for(int32_t side_index = 0; side_index < 6; ++side_index) {
                const BlockSide side = (BlockSide)side_index;
                const BlockTexture tex_slot = get_block_cube_texture(block, side);

                ChunkVaoVertex v[4];

                set_vertex_ao_max(v);
                set_vertex_positions_cube(v, block_offset, side);
                set_vertex_tex_coords(v);
                set_vertex_tex_slots(v, tex_slot);

                push_vertex_quad_cw(mesh_data, v);
            }
        } break;

        case BlockShape::CROSS: {
            const BlockTexture tex_slot = get_block_cross_texture(block);

            ChunkVaoVertex v[4][4];

            /* Set vertex positions */
            set_vertex_positions_cross(v, block_offset);

            /* Set common tex values and push quads */
            for(int32_t index = 0; index < 4; ++index) {
                set_vertex_ao_max(v[index]);
                set_vertex_tex_coords(v[index]);
                set_vertex_tex_slots(v[index], tex_slot);
                push_vertex_quad_cw(mesh_data, v[index]);
            }
        } break;
    }
}
