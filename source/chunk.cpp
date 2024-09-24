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
    this->regenerate_vao();

    /* @todo regenerate neighbouring chunks, this is slow and stupid iguess */
    Chunk *chunk = NULL;
    chunk = m_owner->get_chunk(m_chunk_xz + vec2i{ 1, 0 }, false);
    if(chunk) { chunk->regenerate_vao(); }
    chunk = m_owner->get_chunk(m_chunk_xz + vec2i{ -1, 0 }, false);
    if(chunk) { chunk->regenerate_vao(); }
    chunk = m_owner->get_chunk(m_chunk_xz + vec2i{ 0, 1 }, false);
    if(chunk) { chunk->regenerate_vao(); }
    chunk = m_owner->get_chunk(m_chunk_xz + vec2i{ 0, -1 }, false);
    if(chunk) { chunk->regenerate_vao(); }
}

Block &Chunk::get_block(const vec3i &rel) {
    ASSERT(rel.x >= 0 && rel.x < CHUNK_SIZE_X);
    ASSERT(rel.y >= 0 && rel.y < CHUNK_SIZE_Y);
    ASSERT(rel.z >= 0 && rel.z < CHUNK_SIZE_Z);
    return m_blocks[rel.x][rel.y][rel.z];
}

const Block &Chunk::get_block(const vec3i &rel) const {
    ASSERT(rel.x >= 0 && rel.x < CHUNK_SIZE_X);
    ASSERT(rel.y >= 0 && rel.y < CHUNK_SIZE_Y);
    ASSERT(rel.z >= 0 && rel.z < CHUNK_SIZE_Z);
    return m_blocks[rel.x][rel.y][rel.z];
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

struct BlockSideVertex {
    vec3 position;
    vec3 normal;
    vec2 tex_coord;
};

static constexpr void fill_positions_and_normals(BlockSide side, BlockSideVertex v[4]) {
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
static constexpr void get_atlas_tex_coords(int32_t x, int32_t y, BlockSideVertex v[4]) {
    v[0].tex_coord = vec2{ x * 16.0f / 512.0f, y * 16.0f / 512.0f };
    v[1].tex_coord = vec2{ (x + 1) * 16.0f / 512.0f, y * 16.0f / 512.0f };
    v[2].tex_coord = vec2{ (x + 1) * 16.0f / 512.0f, (y + 1) * 16.0f / 512.0f };
    v[3].tex_coord = vec2{ x * 16.0f / 512.0f, (y + 1) * 16.0f / 512.0f };
}

static constexpr void fill_tex_coords_for_block_type(BlockSide side, BlockType type, BlockSideVertex v[4]) {
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

void Chunk::regenerate_vao(void) {
    std::vector<BlockSideVertex> vertices;
    std::vector<uint32_t> indices;

    auto push_quad = [&](BlockSide side, BlockType type, int32_t x, int32_t y, int32_t z) {
        BlockSideVertex v[4] = { };
        fill_positions_and_normals(side, v);
        fill_tex_coords_for_block_type(side, type, v);
        v[0].position += vec3::make(x, y, z);
        v[1].position += vec3::make(x, y, z);
        v[2].position += vec3::make(x, y, z);
        v[3].position += vec3::make(x, y, z);

        /* Starting index for indices */
        const int32_t start = vertices.size();

        vertices.push_back(v[0]);
        vertices.push_back(v[1]);
        vertices.push_back(v[2]);
        vertices.push_back(v[3]);

        indices.push_back(start + 0);
        indices.push_back(start + 1);
        indices.push_back(start + 2);
        indices.push_back(start + 2);
        indices.push_back(start + 3);
        indices.push_back(start + 0);
    };

    auto is_relative_block_solid = [&] (const vec3i &other_rel) -> bool {
        const Block *other = NULL;
        if(Chunk::is_inside_chunk(other_rel)) {
            other = &this->get_block(other_rel);
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

            other = &neighbour->get_block(block_xyz);
        }
        
        /* If block is transparent, return true */
        switch(other->get_type()) {
            default: break;

            case BlockType::AIR: return false;
        }

        return true;
    };

    for_every_block(x, y, z) {
        const Block &block = this->get_block({ x, y, z });
        if(!block.is_of_type(BlockType::AIR)) {
            if(!is_relative_block_solid({ x, y, z + 1 })) {
                push_quad(BlockSide::Z_POS, block.get_type(), x, y, z);
            }
            if(!is_relative_block_solid({ x, y, z - 1 })) {
                push_quad(BlockSide::Z_NEG, block.get_type(), x, y, z);
            }
            if(!is_relative_block_solid({ x + 1, y, z })) {
                push_quad(BlockSide::X_POS, block.get_type(), x, y, z);
            }
            if(!is_relative_block_solid({ x - 1, y, z })) {
                push_quad(BlockSide::X_NEG, block.get_type(), x, y, z);
            }
            if(!is_relative_block_solid({ x, y + 1, z })) {
                push_quad(BlockSide::Y_POS, block.get_type(), x, y, z);
            }
            if(!is_relative_block_solid({ x, y - 1, z })) {
                push_quad(BlockSide::Y_NEG, block.get_type(), x, y, z);
            }
        }
    }

    BufferLayout layout;
    layout.push_attribute("a_position", 3, BufferDataType::FLOAT);
    layout.push_attribute("a_normal", 3, BufferDataType::FLOAT);
    layout.push_attribute("a_tex_coord", 2, BufferDataType::FLOAT);

    m_chunk_vao.delete_vao();
    m_chunk_vao.create_vao(layout, ArrayBufferUsage::STATIC);
    m_chunk_vao.set_vbo_data(vertices.data(), vertices.size() * sizeof(BlockSideVertex));
    m_chunk_vao.set_ibo_data(indices.data(),  indices.size());
    m_chunk_vao.apply_vao_attributes();
}
