#include "chunk.h"
#include "world.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>

Block::Block(void) {
    this->set_type(BlockType::AIR);
}

void Block::set_type(BlockType type) {
    m_type = type;
}

BlockType Block::get_type(void) const {
    return m_type;
}

bool Block::is_solid(void) const {
    return m_type != BlockType::AIR;
}

bool Block::is_of_type(BlockType type) const {
    return m_type == type;
}

Chunk::Chunk(class World *world, const vec2i &chunk_xz) {
    ASSERT(world);
    m_owner = world;
    m_chunk_xz = chunk_xz;
    memset(m_blocks, 0, CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * sizeof(Block));
}

Chunk::~Chunk(void) {
    m_chunk_vao.delete_vao();
    m_owner = NULL;
}

void Chunk::update_vao(void) {
    this->gen_vao(this->gen_vao_data());

    /* @todo regenerate neighbouring chunks, this is slow and stupid iguess */
    Chunk *chunk = NULL;
    chunk = m_owner->get_chunk(m_chunk_xz + vec2i{ 1, 0 }, false);
    if(chunk) { chunk->gen_vao(chunk->gen_vao_data()); }
    chunk = m_owner->get_chunk(m_chunk_xz + vec2i{ -1, 0 }, false);
    if(chunk) { chunk->gen_vao(chunk->gen_vao_data()); }
    chunk = m_owner->get_chunk(m_chunk_xz + vec2i{ 0, 1 }, false);
    if(chunk) { chunk->gen_vao(chunk->gen_vao_data()); }
    chunk = m_owner->get_chunk(m_chunk_xz + vec2i{ 0, -1 }, false);
    if(chunk) { chunk->gen_vao(chunk->gen_vao_data()); }
}

Block *Chunk::get_block(const vec3i &rel) {
    if(!Chunk::is_inside_chunk(rel)) {
        return NULL;
    }
    return &m_blocks[rel.x][rel.y][rel.z];
}

const Block *Chunk::get_block(const vec3i &rel) const {
    if(!Chunk::is_inside_chunk(rel)) {
        return NULL;
    }
    return &m_blocks[rel.x][rel.y][rel.z];
}

vec2i Chunk::get_coords(void) const {
    return m_chunk_xz;
}

bool Chunk::is_inside_chunk(const vec3i &rel) {
    return(rel.x >= 0 && rel.x < CHUNK_SIZE_X
        && rel.y >= 0 && rel.y < CHUNK_SIZE_Y
        && rel.z >= 0 && rel.z < CHUNK_SIZE_Z);
}

enum class BlockSide {
    Z_POS,
    Z_NEG,
    X_POS,
    X_NEG,
    Y_POS,
    Y_NEG
};

static constexpr void fill_positions_and_normals(BlockSide side, ChunkVaoVertex v[4]) {
    switch(side) {
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
    };
}

// @todo Hardcoded atlas size
static constexpr void get_atlas_tex_coords(int32_t x, int32_t y, ChunkVaoVertex v[4]) {
    v[0].tex_coord = vec2{ x * 16.0f / 512.0f, y * 16.0f / 512.0f };
    v[1].tex_coord = vec2{ (x + 1) * 16.0f / 512.0f, y * 16.0f / 512.0f };
    v[2].tex_coord = vec2{ (x + 1) * 16.0f / 512.0f, (y + 1) * 16.0f / 512.0f };
    v[3].tex_coord = vec2{ x * 16.0f / 512.0f, (y + 1) * 16.0f / 512.0f };
}

static constexpr void fill_tex_coords_for_block_type(BlockSide side, BlockType type, ChunkVaoVertex v[4]) {
    switch(type) {
        default:
        case BlockType::SAND: {
            get_atlas_tex_coords(0, 0, v);
        } break;
        case BlockType::DIRT: {
            get_atlas_tex_coords(1, 0, v);
        } break;
        case BlockType::COBBLESTONE: {
            get_atlas_tex_coords(2, 0, v);
        } break;
        case BlockType::DIRT_WITH_GRASS: {
            switch(side) {
                case BlockSide::Y_NEG: {
                    get_atlas_tex_coords(1, 0, v);
                } break;
                case BlockSide::Y_POS: {
                    get_atlas_tex_coords(1, 2, v);
                } break;
                case BlockSide::X_POS:
                case BlockSide::X_NEG:
                case BlockSide::Z_POS:
                case BlockSide::Z_NEG: {
                    get_atlas_tex_coords(1, 1, v);
                } break;
            }
        } break;
    }
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

ChunkVaoGenData Chunk::gen_vao_data(void) {
    ChunkVaoGenData gen_data = { };

    auto push_quad = [&](BlockSide side, BlockType type, int32_t x, int32_t y, int32_t z) {
        ChunkVaoVertex v[4] = { };
        fill_positions_and_normals(side, v);
        fill_tex_coords_for_block_type(side, type, v);

        v[0].position += vec3::make(x, y, z);
        v[1].position += vec3::make(x, y, z);
        v[2].position += vec3::make(x, y, z);
        v[3].position += vec3::make(x, y, z);

        /* Starting index for indices */
        const int32_t start = gen_data.vertices.size();

        gen_data.vertices.push_back(v[0]);
        gen_data.vertices.push_back(v[1]);
        gen_data.vertices.push_back(v[2]);
        gen_data.vertices.push_back(v[3]);

        gen_data.indices.push_back(start + 0);
        gen_data.indices.push_back(start + 1);
        gen_data.indices.push_back(start + 2);
        gen_data.indices.push_back(start + 2);
        gen_data.indices.push_back(start + 3);
        gen_data.indices.push_back(start + 0);
    };

    // @todo Chunk::get_neighbouring_block(vec2i block, BlockSide side);
    auto is_relative_block_solid = [&] (const vec3i &other_rel) -> bool {
        const Block *other = NULL;
        if(Chunk::is_inside_chunk(other_rel)) {
            other = this->get_block(other_rel);
        } else {
            /* If on the edge in Y - always generate quad */
            if(other_rel.y < 0 || other_rel.y >= CHUNK_SIZE_Y) {
                return false;
            }

            /* Get neighbouring chunk */
            vec2i chunk_offset = { };

            if(other_rel.x < 0) {
                chunk_offset.x = -1;
            } else if(other_rel.x >= CHUNK_SIZE_X) {
                chunk_offset.x = +1;
            }

            if(other_rel.z < 0) {
                chunk_offset.y = -1;
            } else if(other_rel.z >= CHUNK_SIZE_Z) {
                chunk_offset.y = +1;
            }

            const Chunk *neighbour = m_owner->get_chunk(m_chunk_xz + chunk_offset, false);
            if(!neighbour) {
                return false;
            }

            vec3i block_xyz;
            block_xyz.y = other_rel.y;

            if(chunk_offset.x == -1) {
                block_xyz.x = CHUNK_SIZE_X - 1;
            } else if(chunk_offset.x == 1) {
                block_xyz.x = 0;
            } else {
                block_xyz.x = other_rel.x;
            }

            if(chunk_offset.y == -1) {
                block_xyz.z = CHUNK_SIZE_Z - 1;
            } else if(chunk_offset.y == 1) {
                block_xyz.z = 0;
            } else {
                block_xyz.z = other_rel.z;
            }

            other = neighbour->get_block(block_xyz);
        }
        
        /* If block is transparent, return true */
        switch(other->get_type()) {
            default: break;

            case BlockType::AIR: return false;
        }
        return true;
    };

    for_every_block(x, y, z) {
        const Block *block = this->get_block({ x, y, z });
        if(!block->is_of_type(BlockType::AIR)) {
            if(!is_relative_block_solid({ x, y, z + 1 })) {
                push_quad(BlockSide::Z_POS, block->get_type(), x, y, z);
            }
            if(!is_relative_block_solid({ x, y, z - 1 })) {
                push_quad(BlockSide::Z_NEG, block->get_type(), x, y, z);
            }
            if(!is_relative_block_solid({ x + 1, y, z })) {
                push_quad(BlockSide::X_POS, block->get_type(), x, y, z);
            }
            if(!is_relative_block_solid({ x - 1, y, z })) {
                push_quad(BlockSide::X_NEG, block->get_type(), x, y, z);
            }
            if(!is_relative_block_solid({ x, y + 1, z })) {
                push_quad(BlockSide::Y_POS, block->get_type(), x, y, z);
            }
            if(!is_relative_block_solid({ x, y - 1, z })) {
                push_quad(BlockSide::Y_NEG, block->get_type(), x, y, z);
            }
        }
    }

    gen_data.chunk = this->m_chunk_xz;
    return gen_data;
}
