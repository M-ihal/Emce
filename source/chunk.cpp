#include "chunk.h"
#include "world.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glew.h>

Chunk::Chunk(class World *world, vec2i chunk_xz) {
    ASSERT(world);
    m_owner = world;
    m_chunk_xz = chunk_xz;
    memset(m_blocks, 0, CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * sizeof(Block));
}

Chunk::~Chunk(void) {
    m_chunk_vao.delete_vao();
    m_owner = NULL;
}

const VertexArray &Chunk::get_vao(void) {
    return m_chunk_vao;
}

void Chunk::set_block(const vec3i &rel, BlockType type) {
    if(is_inside_chunk(rel)) {
        m_blocks[rel.x][rel.y][rel.z].type = type;
    }
}

Block *Chunk::get_block(const vec3i &rel) {
    Block *block = NULL;
    if(is_inside_chunk(rel)) {
        block = &m_blocks[rel.x][rel.y][rel.z];
    }
    return block;
}

Block *Chunk::get_block_neighbour(const vec3i &rel, BlockSide side, bool check_other_chunk) {
    Block *block = NULL;
    vec3i nb_rel = rel + get_block_side_dir(side);
    if(is_inside_chunk(nb_rel)) {
        block = &m_blocks[nb_rel.x][nb_rel.y][nb_rel.z];
    } else if(check_other_chunk && nb_rel.y >= 0 && nb_rel.y < CHUNK_SIZE_Y) {
        const vec3i nb_abs = block_position_from_relative(nb_rel, m_chunk_xz);
        WorldPosition nb_p = WorldPosition::from_block(nb_abs);
        Chunk *nb_chunk = this->m_owner->get_chunk(nb_p.chunk);
        if(nb_chunk != NULL) {
            block = nb_chunk->get_block(nb_p.block_rel);
        }
    }
    return block;
}

vec2i Chunk::get_coords(void) {
    return m_chunk_xz;
}

void Chunk::gen_vao(const ChunkVaoGenData &gen_data) {
    BufferLayout layout;
    layout.push_attribute("a_position",  3, BufferDataType::FLOAT);
    layout.push_attribute("a_normal",    3, BufferDataType::FLOAT);
    layout.push_attribute("a_tex_coord", 2, BufferDataType::FLOAT);

    m_chunk_vao.delete_vao();
    m_chunk_vao.create_vao(layout, ArrayBufferUsage::STATIC);
    m_chunk_vao.set_vbo_data(gen_data.vertices.data(), gen_data.vertices.size() * sizeof(ChunkVaoVertex));
    m_chunk_vao.set_ibo_data(gen_data.indices.data(),  gen_data.indices.size());
    m_chunk_vao.apply_vao_attributes();
}

static constexpr void calc_atlas_tex_coords(int32_t x, int32_t y, ChunkVaoVertex v[4]) {
    v[0].tex_coord = vec2{ x * 16.0f / 512.0f, y * 16.0f / 512.0f };
    v[1].tex_coord = vec2{ (x + 1) * 16.0f / 512.0f, y * 16.0f / 512.0f };
    v[2].tex_coord = vec2{ (x + 1) * 16.0f / 512.0f, (y + 1) * 16.0f / 512.0f };
    v[3].tex_coord = vec2{ x * 16.0f / 512.0f, (y + 1) * 16.0f / 512.0f };
}

static void push_block_vao_quad(ChunkVaoVertex quad_vertices[4], std::vector<ChunkVaoVertex> &vertices, std::vector<uint32_t> &indices) {
    const int32_t start = vertices.size();

    vertices.push_back(quad_vertices[0]);
    vertices.push_back(quad_vertices[1]);
    vertices.push_back(quad_vertices[2]);
    vertices.push_back(quad_vertices[3]);

    indices.push_back(start + 0);
    indices.push_back(start + 1);
    indices.push_back(start + 2);
    indices.push_back(start + 2);
    indices.push_back(start + 3);
    indices.push_back(start + 0);
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

    v[0].position += vec3::make(offset);
    v[1].position += vec3::make(offset);
    v[2].position += vec3::make(offset);
    v[3].position += vec3::make(offset);
}

static void fill_block_cube_vao_tex_coords(ChunkVaoVertex v[4], BlockType type, BlockSide side) {
    switch(type) {
        default: INVALID_CODE_PATH; break;

        case BlockType::SAND: {
            calc_atlas_tex_coords(0, 0, v);
        } break;

        case BlockType::DIRT: {
            calc_atlas_tex_coords(1, 0, v);
        } break;

        case BlockType::DIRT_WITH_GRASS: {
            switch(side) {
                case BlockSide::Y_NEG: {
                    calc_atlas_tex_coords(1, 0, v);
                } break;
                case BlockSide::Y_POS: {
                    calc_atlas_tex_coords(1, 2, v);
                } break;
                case BlockSide::X_POS:
                case BlockSide::X_NEG:
                case BlockSide::Z_POS:
                case BlockSide::Z_NEG: {
                    calc_atlas_tex_coords(1, 1, v);
                } break;
            }
        } break;

        case BlockType::COBBLESTONE: {
            calc_atlas_tex_coords(2, 0, v);
        } break;

        case BlockType::STONE: {
            calc_atlas_tex_coords(2, 1, v);
        } break;

        case BlockType::TREE_LOG: {
            switch(side) {
                case BlockSide::Y_POS:
                case BlockSide::Y_NEG: {
                    calc_atlas_tex_coords(7, 0, v);
                } break;

                case BlockSide::X_POS:
                case BlockSide::X_NEG:
                case BlockSide::Z_POS:
                case BlockSide::Z_NEG: {
                    calc_atlas_tex_coords(7, 1, v);
                } break;
            };
        } break;

        case BlockType::TREE_LEAVES: {
            calc_atlas_tex_coords(7, 2, v);
        } break;
        
        case BlockType::GLASS: {
            calc_atlas_tex_coords(0, 1, v);
        } break;

        case BlockType::WATER: {
            calc_atlas_tex_coords(31, 0, v);
        } break;
    }
}

static void fill_block_cross_vao_vertex(ChunkVaoVertex v[2][4], const vec3i &offset) {

}

static void fill_block_cross_vao_tex_coords(ChunkVaoVertex v[2][4], BlockType type) {

}

static void push_block_vao_data(BlockType type, std::vector<ChunkVaoVertex> &vertices, std::vector<uint32_t> &indices, const vec3i &block_rel, bool opaque_neighbours[BLOCK_SIDE_COUNT]) {

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
            for(int32_t index = 0; index < BLOCK_SIDE_COUNT; ++index) {
                ChunkVaoVertex v[4] = { };
                BlockSide side = (BlockSide)index;
                if(!opaque_neighbours[index]) {
                    fill_block_cube_vao_vertex(v, side, block_rel);
                    fill_block_cube_vao_tex_coords(v, type, side);
                    push_block_vao_quad(v, vertices, indices);
                }
            }
        } break;

        case CROSS: {
            ChunkVaoVertex v[2][4] = { };
            fill_block_cross_vao_vertex(v, block_rel);
            fill_block_cross_vao_tex_coords(v, type);
            push_block_vao_quad(v[0], vertices, indices);
            push_block_vao_quad(v[1], vertices, indices);
        } break;
    }
}

void gen_single_block_vao_data(BlockType type, std::vector<ChunkVaoVertex> &vertices, std::vector<uint32_t> &indices) {
    bool opaques[BLOCK_SIDE_COUNT] = {
        false, false, false, 
        false, false, false
    };

    push_block_vao_data(type, vertices, indices, { 0, 0, 0 }, opaques);
    for(ChunkVaoVertex &v : vertices) {
        v.position -= vec3::make(0.5f);
    }
}

ChunkVaoGenData Chunk::gen_vao_data(void) {
    ChunkVaoGenData gen_data = { .chunk = this->m_chunk_xz };

    for_every_block(x, y, z) {
        const vec3i block_rel = { x, y, z };

        Block *block = this->get_block(block_rel);
        ASSERT(block);

        const uint32_t flags = get_block_flags(block->type);
        if(flags & IS_NOT_VISIBLE) {
            continue;
        }

        /* Get array of opaque neighbouring blocks */
        bool opaque_neighbours[BLOCK_SIDE_COUNT] = { };
        for(int32_t side = 0; side < BLOCK_SIDE_COUNT; ++side) {
            Block *neighbour = this->get_block_neighbour(block_rel, (BlockSide)side, true);
            if(neighbour != NULL) {
                uint32_t nb_flags = get_block_flags(neighbour->type);
                opaque_neighbours[side] = !(nb_flags & IS_NOT_VISIBLE) && !(nb_flags & HAS_TRANSPARENCY);

                if(flags & HAS_TRANSPARENCY && nb_flags & HAS_TRANSPARENCY && !(nb_flags & IS_NOT_VISIBLE)) {
                    opaque_neighbours[side] = true;
                }
            }       
        }

        push_block_vao_data(block->type, gen_data.vertices, gen_data.indices, block_rel, opaque_neighbours);
    }

    return gen_data;
}

