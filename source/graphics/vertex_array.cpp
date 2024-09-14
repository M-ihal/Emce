#include "vertex_array.h"

#include <glew.h>
#include <stdio.h>
#include <cstring>

static constexpr int32_t gl_from_array_buffer_usage(ArrayBufferUsage usage) {
    switch(usage) {
        default: INVALID_CODE_PATH;
        case ArrayBufferUsage::STATIC:  return GL_STATIC_DRAW;
        case ArrayBufferUsage::DYNAMIC: return GL_DYNAMIC_DRAW;
    }
}

static constexpr int32_t gl_from_array_buffer_data_type(ArrayBufferDataType type) {
    switch(type) {
        default: INVALID_CODE_PATH;
        case ArrayBufferDataType::INT:   return GL_INT;
        case ArrayBufferDataType::FLOAT: return GL_FLOAT;
    }
}

static constexpr int32_t size_of_array_buffer_data_type(ArrayBufferDataType type) {
    switch(type) {
        default: INVALID_CODE_PATH; return -1;
        case ArrayBufferDataType::INT:
        case ArrayBufferDataType::FLOAT: {
            return 4;
        }
    }
}

VertexArray::VertexArray(void) {
    ZERO_STRUCT(*this);
}

void VertexArray::bind_no_vao(void) {
    GL_CHECK(glBindVertexArray(0));
}

void VertexArray::bind_vao(void) const {
    ASSERT(m_vao_id);
    GL_CHECK(glBindVertexArray(m_vao_id));
}

void VertexArray::create_vao(const BufferLayout &vbo_layout, ArrayBufferUsage usage) {
    m_vbo_layout = vbo_layout;
    m_usage = usage;

    const int32_t gl_usage = gl_from_array_buffer_usage(m_usage);

    /* Generate vao ID */
    GL_CHECK(glGenVertexArrays(1, &m_vao_id));
    if(!m_vao_id) {
        fprintf(stderr, "[error] VAO: Failed to generate vertex array id.\n");
        return;
    } else {
        GL_CHECK(glBindVertexArray(m_vao_id));
        fprintf(stdout, "[info] VAO: Created vertex array, ID: %d (%p)\n", m_vao_id, this);
    }

    /* Generate vbo ID */
    GL_CHECK(glGenBuffers(1, &m_vbo_id));
    if(!m_vbo_id) {
        fprintf(stderr, "[error] VAO: Failed to generate vertex buffer id.\n");
        this->delete_vao();
        return;
    } else {
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, m_vbo_id));
        fprintf(stdout, "[info] VAO: VBO ID: %d attached to VAO ID: %d (%p)\n", m_vbo_id, m_vao_id, this);
    }

    /* Generate ibo ID */
    GL_CHECK(glGenBuffers(1, &m_ibo_id));
    if(!m_ibo_id) {
        fprintf(stderr, "[error] VAO: Failed to generate index buffer id.\n");
        this->delete_vao();
        return;
    } else {
        GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo_id));
        fprintf(stdout, "[info] VAO: IBO ID: %d attached to VAO ID: %d (%p)\n", m_ibo_id, m_vao_id, this);
    }

    this->bind_no_vao();
}
    
void VertexArray::delete_vao(void) {
    if(m_ibo_id) {
        GL_CHECK(glDeleteBuffers(1, &m_ibo_id));
        fprintf(stdout, "[info] VAO: Deleted attached IBO, ID: %d\n", m_ibo_id);
        m_ibo_id = 0;
    }

    if(m_vbo_id) {
        GL_CHECK(glDeleteBuffers(1, &m_vbo_id));
        fprintf(stdout, "[info] VAO: Deleted attached VBO, ID: %d\n", m_vbo_id);
        m_vbo_id = 0;
    }

    if(m_vao_id) {
        glDeleteVertexArrays(1, &m_vao_id);
        fprintf(stdout, "[info] VAO: Deleted VAO, ID: %d (%p)\n", m_vbo_id, this);
        m_vao_id = 0;
    }
}

void VertexArray::apply_vao_attributes(void) const {
    this->bind_vao();

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, m_vbo_id));

    for(int32_t att_index = 0; att_index < m_vbo_layout.m_next_attribute; ++att_index) {
        const BufferLayout::Attribute *att = &m_vbo_layout.m_attributes[att_index];
        const int32_t location = att_index;

        /* TODO glVertexAttribIPointer? */
        GL_CHECK(glEnableVertexAttribArray(location));
        GL_CHECK(glVertexAttribPointer(location, att->count, att->data_type, GL_FALSE, m_vbo_layout.m_stride, (void *)((uint64_t)att->offset)));
    }

    VertexArray::bind_no_vao();
}

void VertexArray::set_vbo_data(const void *data, size_t size) {
    ASSERT(m_vao_id);
    ASSERT(m_vbo_id);
    const int32_t gl_usage = gl_from_array_buffer_usage(m_usage);

    this->bind_vao();
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, m_vbo_id));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, size, data, gl_usage));
    m_vbo_size = size;
}

void VertexArray::set_ibo_data(const uint32_t *data, int32_t count) {
    ASSERT(m_vao_id);
    ASSERT(m_ibo_id);
    const int32_t gl_usage = gl_from_array_buffer_usage(m_usage);

    this->bind_vao();
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo_id));
    GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), data, gl_usage));
    m_ibo_count = count;
}

void VertexArray::upload_vbo_data(const void *data, size_t size, int32_t offset) const {
    ASSERT(m_vao_id);
    ASSERT(m_vbo_id);
    ASSERT(m_vbo_size >= size);

    this->bind_vao();
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, m_vbo_id));
    GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER, offset, size, data));
}

void VertexArray::upload_ibo_data(const uint32_t *data, int32_t count, int32_t offset) const {
    ASSERT(m_vao_id);
    ASSERT(m_ibo_id);
    ASSERT(m_ibo_count >= count);

    this->bind_vao();
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo_id));
    GL_CHECK(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, count * sizeof(uint32_t), data));
}

int32_t VertexArray::get_vbo_size(void) const {
    return m_vbo_size;
}

int32_t VertexArray::get_ibo_count(void) const {
    return m_ibo_count;
}

/*
    class BufferLayout
*/

BufferLayout::BufferLayout(void) {
    this->reset();
}

void BufferLayout::reset(void) {
    m_stride = 0;
    m_combined_count = 0;
    m_next_attribute = 0;
    ZERO_ARRAY(m_attributes);
}

void BufferLayout::push_attribute(const char *name, int32_t count, int32_t data_type, int32_t data_type_size) {
    ASSERT(m_next_attribute < ARRAY_COUNT(m_attributes));
    const int32_t att_index = m_next_attribute;

    Attribute *att = &m_attributes[att_index];
    strcpy_s(att->name, ARRAY_COUNT(att->name), name); // @check
    att->count = count;
    att->offset = m_stride;
    att->data_type = data_type;
    att->data_type_size = data_type_size;

    m_stride += data_type_size * count;
    m_combined_count += count;
    m_next_attribute += 1;
}


#if 0

VertexArray::VertexArray(void) {
    m_vao_id = 0;
    m_vbo_count = 0;
    ZERO_ARRAY(m_vbo_id);
    ZERO_ARRAY(m_vbo_layout);
    m_ibo_id = 0;
    m_ibo_index_count = 0;
}

VertexArray::~VertexArray(void) {
    if(m_vbo_count) {
        for(int32_t vbo_idx = 0; vbo_idx < m_vbo_count; ++vbo_idx) {
            GL_CHECK(glDeleteBuffers(1, &m_vbo_id[vbo_idx]));
            fprintf(stdout, "[info] VAO: Deleted attached VBO, ID: %d\n", m_vbo_id[vbo_idx]);
            m_vbo_id[vbo_idx] = 0;
        }
    }

    if(m_ibo_id) {
        GL_CHECK(glDeleteBuffers(1, &m_ibo_id));
        fprintf(stdout, "[info] VAO: Deleted attached IBO, ID: %d\n", m_ibo_id);
        m_ibo_id = 0;
    }

    if(m_vao_id) {
        glDeleteVertexArrays(1, &m_vao_id);
        m_vao_id = 0;

        fprintf(stdout, "[info] VAO: Deleted vertex array (%p)\n", this);
    }
}

void VertexArray::bind_no_vao(void) {
    GL_CHECK(glBindVertexArray(0));
}

void VertexArray::bind_vao(void) {
    ASSERT(m_vao_id);
    GL_CHECK(glBindVertexArray(m_vao_id));
}

void VertexArray::generate_vao_id(void) {
    ASSERT(!m_vao_id);

    GL_CHECK(glGenVertexArrays(1, &m_vao_id));
    if(!m_vao_id) {
        fprintf(stderr, "[error] VAO: Failed to generate vertex array id.\n");
        return;
    }

    VertexArray::bind_no_vao();
    fprintf(stdout, "[info] VAO: Created vertex array, ID: %d (%p)\n", m_vao_id, this);
}

void VertexArray::apply_vertex_attributes(void) {
    ASSERT(m_vao_id);

    this->bind_vao();

    int32_t next_location = 0;

    /* Loop through all attached vbos and set every attribute from its' layout */
    for(int32_t vbo_index = 0; vbo_index < m_vbo_count; ++vbo_index) {
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, m_vbo_id[vbo_index]));

        const BufferLayout *layout = &m_vbo_layout[vbo_index];

        for(int32_t att_index = 0; att_index < layout->m_next_attribute; ++att_index) {
            const BufferLayout::Attribute *att = &layout->m_attributes[att_index];
            const int32_t location = next_location + att_index;

            /* TODO glVertexAttribIPointer? */
            GL_CHECK(glEnableVertexAttribArray(location));
            GL_CHECK(glVertexAttribPointer(location, att->count, att->data_type, GL_FALSE, layout->m_stride, (void *)((uint64_t)att->offset)));
        }
    }

    VertexArray::bind_no_vao();
}

void VertexArray::add_vertex_buffer(const void *data, size_t data_size, ArrayBufferUsage usage, const BufferLayout &layout) {
    ASSERT(m_vao_id);
    ASSERT(m_vbo_count < ARRAY_COUNT(m_vbo_id));
    ASSERT(data && data_size);

    this->bind_vao();

    uint32_t vbo_id = 0;
    GL_CHECK(glGenBuffers(1, &vbo_id));
    if(!vbo_id) {
        fprintf(stderr, "[error] VAO: Failed to create vertex buffer id.\n");
        INVALID_CODE_PATH;
        return;
    }

    const int32_t gl_usage = gl_from_array_buffer_usage(usage);

    /* Setup buffer & upload data */
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_id));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, data_size, data, gl_usage));
    oo
        o


    const int32_t vbo_idx = m_vbo_count++;
    m_vbo_id[vbo_idx] = vbo_id;
    m_vbo_layout[vbo_idx] = layout;

    fprintf(stdout, "[info] VAO: VBO ID: %d attached to VAO ID: %d (%p)\n", vbo_id, m_vao_id, this);
    VertexArray::bind_no_vao();
}

void VertexArray::add_index_buffer(const uint32_t *index_data, size_t index_count, ArrayBufferUsage usage) {
    ASSERT(m_vao_id);
    ASSERT(m_ibo_id == 0);
    ASSERT(index_data && index_count);
    
    this->bind_vao();

    uint32_t ibo_id = 0;
    GL_CHECK(glGenBuffers(1, &ibo_id));
    if(!ibo_id) {
        fprintf(stderr, "[error] VAO: Failed to create index buffer id.\n");
        INVALID_CODE_PATH;
        return;
    }

    const int32_t gl_usage = gl_from_array_buffer_usage(usage);
    const size_t data_size = index_count * 4;

    /* Setup buffer & upload data */
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_id));
    GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, data_size, index_data, gl_usage));

    m_ibo_id = ibo_id;
    m_ibo_index_count = index_count;

    fprintf(stdout, "[info] VAO: IBO ID: %d attached to VAO ID: %d (%p)\n", ibo_id, m_vao_id, this);
    VertexArray::bind_no_vao();
}

void VertexArray::set_vertex_buffer_data(uint32_t vbo_index, const void *data, size_t data_size, int32_t offset) {
    ASSERT(m_vao_id);
    ASSERT(vbo_index < m_vbo_count);
    ASSERT(m_vbo_id[vbo_index]);

    this->bind_vao();
    
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, m_vbo_id[vbo_index]));
    GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER, offset, data_size, data));
}

void VertexArray::set_index_buffer_data(const uint32_t *index_data, size_t index_count, int32_t offset) {
    ASSERT(m_vao_id);
    ASSERT(m_ibo_id);

    this->bind_vao();
    
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo_id));
    GL_CHECK(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, index_count * sizeof(uint32_t), index_data))
}

uint32_t VertexArray::index_count(void) const {
    return m_ibo_index_count;
}

/*
    BufferLayout
*/

BufferLayout::BufferLayout(void) {
    this->reset();
}

void BufferLayout::reset(void) {
    m_stride = 0;
    m_combined_count = 0;
    m_next_attribute = 0;
    ZERO_ARRAY(m_attributes);
}

void BufferLayout::push_attribute(const char *name, int32_t count, int32_t data_type, int32_t data_type_size) {
    ASSERT(m_next_attribute < ARRAY_COUNT(m_attributes));

    const int32_t att_index = m_next_attribute;

    Attribute *att = &m_attributes[att_index];
    strcpy_s(att->name, ARRAY_COUNT(att->name), name); // @check
    att->count = count;
    att->offset = m_stride;
    att->data_type = data_type;
    att->data_type_size = data_type_size;

    m_stride += data_type_size * count;
    m_combined_count += count;
    m_next_attribute += 1;
}

VertexArray gen_cube_vao(void) {
    struct CubeVertex {
        vec3 position;
        vec3 normal;
        vec2 tex_coord;
    };

    const CubeVertex vertices[] = {
        { -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f }, // Bottom-left
        {  0.5f,  0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f }, // top-right
        {  0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f }, // bottom-right         
        {  0.5f,  0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f }, // top-right
        { -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f }, // bottom-left
        { -0.5f,  0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f }, // top-left
                                                                // Front face
        { -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f }, // bottom-left
        {  0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f }, // bottom-right
        {  0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f }, // top-right
        {  0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f }, // top-right
        { -0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f }, // top-left
        { -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f }, // bottom-left
                                                               // Left face
        { -0.5f,  0.5f,  0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f }, // top-right
        { -0.5f,  0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f }, // top-left
        { -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f }, // bottom-left
        { -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f }, // bottom-left
        { -0.5f, -0.5f,  0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f }, // bottom-right
        { -0.5f,  0.5f,  0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f }, // top-right
                                                                        // Right face
        { 0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f },   // top-left
        { 0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f },   // bottom-right
        { 0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f },   // top-right         
        { 0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f },   // bottom-right
        { 0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f },   // top-left
        { 0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f },   // bottom-left     
                                                                // Bottom face
        { -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f }, // top-right
        {  0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f }, // top-left
        {  0.5f, -0.5f,  0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f }, // bottom-left
        {  0.5f, -0.5f,  0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f }, // bottom-left
        { -0.5f, -0.5f,  0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f }, // bottom-right
        { -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f }, // top-right
                                                                // Top face
        { -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f },  // top-left
        {  0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f },  // bottom-right
        {  0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f },  // top-right     
        {  0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f },  // bottom-right
        { -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f },  // top-left
        { -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f }   // bottom-left 
    };

    const uint32_t indices[] = {
         0,  1,  2,
         3,  4,  5,
         6,  7,  8,
         9, 10, 11,
        12, 13, 14,
        15, 16, 17,
        18, 19, 20,
        21, 22, 23,
        24, 25, 26,
        27, 28, 29,
        30, 31, 32,
        33, 34, 35
    };

    BufferLayout layout;
    layout.push_attribute("a_position", 3, GL_FLOAT, 4);
    layout.push_attribute("a_normal", 3, GL_FLOAT, 4);
    layout.push_attribute("a_tex_coord", 2, GL_FLOAT, 4);

    VertexArray vao;
    vao.generate_vao_id();
    vao.add_vertex_buffer(vertices, ARRAY_COUNT(vertices) * sizeof(CubeVertex), ArrayBufferUsage::STATIC, layout);
    vao.add_index_buffer(indices, ARRAY_COUNT(indices), ArrayBufferUsage::STATIC);
    vao.apply_vertex_attributes();
    return vao;
}
#endif
