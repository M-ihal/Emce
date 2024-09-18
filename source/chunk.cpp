#include "chunk.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

Chunk::Chunk(void) {
    memset(m_blocks, 0, CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * sizeof(Block));
    this->update_chunk_vao();
}

Chunk::~Chunk(void) {
    m_chunk_vao.delete_vao();
}

VertexArray stupid_regen_chunk_vao(const Chunk &chunk);

void Chunk::update_chunk_vao(void) {
    m_chunk_vao.delete_vao();
    m_chunk_vao = stupid_regen_chunk_vao(*this);
}

Block &Chunk::get_block(int32_t rel_x, int32_t rel_y, int32_t rel_z) {
    ASSERT(rel_x >= 0 && rel_x < CHUNK_SIZE_X);
    ASSERT(rel_y >= 0 && rel_y < CHUNK_SIZE_Y);
    ASSERT(rel_z >= 0 && rel_z < CHUNK_SIZE_Z);
    return m_blocks[rel_x][rel_y][rel_z];
}

const Block &Chunk::get_block(int32_t rel_x, int32_t rel_y, int32_t rel_z) const {
    ASSERT(rel_x >= 0 && rel_x < CHUNK_SIZE_X);
    ASSERT(rel_y >= 0 && rel_y < CHUNK_SIZE_Y);
    ASSERT(rel_z >= 0 && rel_z < CHUNK_SIZE_Z);
    return m_blocks[rel_x][rel_y][rel_z];
}

bool Chunk::is_inside_chunk(int32_t rel_x, int32_t rel_y, int32_t rel_z) {
    return(rel_x >= 0 && rel_x < CHUNK_SIZE_X
        && rel_y >= 0 && rel_y < CHUNK_SIZE_Y
        && rel_z >= 0 && rel_z < CHUNK_SIZE_Z);
}

#include <vector>

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

void fill_positions_and_normals(BlockSide side, BlockSideVertex v[4]) {
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
void get_atlas_tex_coords(int32_t x, int32_t y, BlockSideVertex v[4]) {
    v[0].tex_coord = vec2{ x * 16.0f / 512.0f, y * 16.0f / 512.0f };
    v[1].tex_coord = vec2{ (x + 1) * 16.0f / 512.0f, y * 16.0f / 512.0f };
    v[2].tex_coord = vec2{ (x + 1) * 16.0f / 512.0f, (y + 1) * 16.0f / 512.0f };
    v[3].tex_coord = vec2{ x * 16.0f / 512.0f, (y + 1) * 16.0f / 512.0f };
}

void fill_tex_coords_for_block_type(BlockSide side, BlockType type, BlockSideVertex v[4]) {
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
    }
}

VertexArray stupid_regen_chunk_vao(const Chunk &chunk) {

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

    for_every_block(x, y, z) {
        const Block &block = chunk.get_block(x, y, z);
        if(!block.is_of_type(BlockType::AIR)) {
            if(!(Chunk::is_inside_chunk(x, y, z + 1) && !chunk.get_block(x, y, z + 1).is_of_type(BlockType::AIR))) {
                push_quad(BlockSide::Z_POS, block.get_type(), x, y, z);
            }
            if(!(Chunk::is_inside_chunk(x, y, z - 1) && !chunk.get_block(x, y, z - 1).is_of_type(BlockType::AIR))) {
                push_quad(BlockSide::Z_NEG, block.get_type(), x, y, z);
            }
            if(!(Chunk::is_inside_chunk(x + 1, y, z) && !chunk.get_block(x + 1, y, z).is_of_type(BlockType::AIR))) {
                push_quad(BlockSide::X_POS, block.get_type(), x, y, z);
            }
            if(!(Chunk::is_inside_chunk(x - 1, y, z) && !chunk.get_block(x - 1, y, z).is_of_type(BlockType::AIR))) {
                push_quad(BlockSide::X_NEG, block.get_type(), x, y, z);
            }
            if(!(Chunk::is_inside_chunk(x, y + 1, z) && !chunk.get_block(x, y + 1, z).is_of_type(BlockType::AIR))) {
                push_quad(BlockSide::Y_POS, block.get_type(), x, y, z);
            }
            if(!(Chunk::is_inside_chunk(x, y - 1, z) && !chunk.get_block(x, y - 1, z).is_of_type(BlockType::AIR))) {
                push_quad(BlockSide::Y_NEG, block.get_type(), x, y, z);
            }
        }
    }

    BufferLayout layout;
    layout.push_attribute("a_position", 3, BufferDataType::FLOAT);
    layout.push_attribute("a_normal", 3, BufferDataType::FLOAT);
    layout.push_attribute("a_tex_coord", 2, BufferDataType::FLOAT);

    VertexArray vao;
    vao.create_vao(layout, ArrayBufferUsage::STATIC);
    vao.set_vbo_data(vertices.data(), vertices.size() * sizeof(BlockSideVertex));
    vao.set_ibo_data(indices.data(),  indices.size());
    vao.apply_vao_attributes();
    return vao;
}
