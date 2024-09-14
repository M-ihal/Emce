#include "chunk.h"

#include <stdlib.h>

// @TEMP
#include <glew.h>

Block::Block(void) {
    this->set_type(BlockType::AIR);
}

bool Block::is_of_type(BlockType type) const {
    return m_type == type;
}

void Block::set_type(BlockType type) {
    m_type = type;
}

Chunk::Chunk(void) {
    for_every_block(x, y, z) {
       Block &block = this->get_block(x, y, z);

#if 0
       if(rand() % 4 == 0) {
           block.set_type(BlockType::SAND);
       } else {
           block.set_type(BlockType::AIR);
       }
#else
       block.set_type(BlockType::SAND);
#endif
    }

    this->update_chunk_vao();
}

Chunk::~Chunk(void) {

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

void Chunk::render(const Shader &shader, const Texture &texture) {
    shader.use_program();
    shader.upload_mat4("u_model", mat4::identity().e);
    shader.upload_int("u_texture", 0);
    texture.bind_texture_unit(0);

    m_chunk_vao.bind_vao();
    GL_CHECK(glDrawElements(GL_TRIANGLES, m_chunk_vao.get_ibo_count(), GL_UNSIGNED_INT, 0));
}

#include <vector>

VertexArray stupid_regen_chunk_vao(const Chunk &chunk) {

    struct Vertex { vec3 position; vec3 normal; vec2 tex_coord; };

    // Z negative normal
    const Vertex v_z_n[] = {
        { -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f }, // Bottom-left
        {  0.5f,  0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f }, // top-right
        {  0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f }, // bottom-right         
        {  0.5f,  0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f }, // top-right
        { -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f }, // bottom-left
        { -0.5f,  0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f }, // top-left
    };
    
    const Vertex v_z_p[] = {
        { -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f }, // bottom-left
        {  0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f }, // bottom-right
        {  0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f }, // top-right
        {  0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f }, // top-right
        { -0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f }, // top-left
        { -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f }, // bottom-left
    };

    const Vertex v_x_n[] = {
        { -0.5f,  0.5f,  0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f }, // top-right
        { -0.5f,  0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f }, // top-left
        { -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f }, // bottom-left
        { -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f }, // bottom-left
        { -0.5f, -0.5f,  0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f }, // bottom-right
        { -0.5f,  0.5f,  0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f }, // top-right
    };

    const Vertex v_x_p[] = {
        { 0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f },   // top-left
        { 0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f },   // bottom-right
        { 0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f },   // top-right         
        { 0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f },   // bottom-right
        { 0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f },   // top-left
        { 0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f },   // bottom-left     
    };

    const Vertex v_y_n[] = {
        { -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f }, // top-right
        {  0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f }, // top-left
        {  0.5f, -0.5f,  0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f }, // bottom-left
        {  0.5f, -0.5f,  0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f }, // bottom-left
        { -0.5f, -0.5f,  0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f }, // bottom-right
        { -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f }, // top-right
    };

    const Vertex v_y_p[] = {
        { -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f },  // top-left
        {  0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f },  // bottom-right
        {  0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f },  // top-right     
        {  0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f },  // bottom-right
        { -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f },  // top-left
        { -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f }   // bottom-left 
    };

    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;

    auto push_quad = [&](const Vertex v[6], int32_t x, int32_t y, int32_t z) {
        for(int32_t i = 0; i < 6; ++i) {
            Vertex vert = v[i];
            vert.position += vec3{ float(x), float(y), float(z) };
            vertices.push_back(vert);
        }

        const int32_t start = indices.size();
        indices.push_back(start + 0);
        indices.push_back(start + 1);
        indices.push_back(start + 2);
        indices.push_back(start + 3);
        indices.push_back(start + 4);
        indices.push_back(start + 5);
    };

    for_every_block(x, y, z) {
        const Block &block = chunk.get_block(x, y, z);
        if(block.is_of_type(BlockType::SAND)) {
            push_quad(v_z_p, x, y, z);
            push_quad(v_z_n, x, y, z);
            push_quad(v_x_p, x, y, z);
            push_quad(v_x_n, x, y, z);
            push_quad(v_y_p, x, y, z);
            push_quad(v_y_n, x, y, z);
        }
    }

    BufferLayout layout;
    layout.push_attribute("a_position", 3, GL_FLOAT, 4);
    layout.push_attribute("a_normal", 3, GL_FLOAT, 4);
    layout.push_attribute("a_tex_coord", 2, GL_FLOAT, 4);

    VertexArray vao;
    vao.create_vao(layout, ArrayBufferUsage::STATIC);
    vao.set_vbo_data(vertices.data(), vertices.size() * sizeof(Vertex));
    vao.set_ibo_data(indices.data(),  indices.size());
    vao.apply_vao_attributes();
    return vao;
}
