#include "vertex_array.h"

#include <glew.h>
#include <stdio.h>
#include <cstring>

static inline constexpr int32_t gl_from_array_buffer_usage(ArrayBufferUsage usage) {
    switch(usage) {
        default: INVALID_CODE_PATH;     return -1;
        case ArrayBufferUsage::STATIC:  return GL_STATIC_DRAW;
        case ArrayBufferUsage::DYNAMIC: return GL_DYNAMIC_DRAW;
    }
}

static inline constexpr int32_t gl_from_buffer_data_type(BufferDataType type) {
    switch(type) {
        default: INVALID_CODE_PATH; return -1;
        case BufferDataType::INT:   return GL_INT;
        case BufferDataType::FLOAT: return GL_FLOAT;
    }
}

static inline constexpr int32_t size_of_buffer_data_type(BufferDataType type) {
    switch(type) {
        default: INVALID_CODE_PATH; return -1;
        case BufferDataType::INT:   return 4;
        case BufferDataType::FLOAT: return 4;
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
        // fprintf(stdout, "[info] VAO: Created vertex array, ID: %d\n", m_vao_id);
    }

    /* Generate vbo ID */
    GL_CHECK(glGenBuffers(1, &m_vbo_id));
    if(!m_vbo_id) {
        fprintf(stderr, "[error] VAO: Failed to generate vertex buffer id.\n");
        this->delete_vao();
        return;
    } else {
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, m_vbo_id));
        // fprintf(stdout, "[info] VAO: VBO ID: %d attached to VAO ID: %d\n", m_vbo_id, m_vao_id);
    }

    /* Generate ibo ID */
    GL_CHECK(glGenBuffers(1, &m_ibo_id));
    if(!m_ibo_id) {
        fprintf(stderr, "[error] VAO: Failed to generate index buffer id.\n");
        this->delete_vao();
        return;
    } else {
        GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo_id));
        // fprintf(stdout, "[info] VAO: IBO ID: %d attached to VAO ID: %d\n", m_ibo_id, m_vao_id);
    }

    this->bind_no_vao();
}
    
void VertexArray::delete_vao(void) {
    if(m_ibo_id) {
        // fprintf(stdout, "[info] VAO: Deleted attached IBO, ID: %d\n", m_ibo_id);
        GL_CHECK(glDeleteBuffers(1, &m_ibo_id));
        m_ibo_id = 0;
    }

    if(m_vbo_id) {
        // fprintf(stdout, "[info] VAO: Deleted attached VBO, ID: %d\n", m_vbo_id);
        GL_CHECK(glDeleteBuffers(1, &m_vbo_id));
        m_vbo_id = 0;
    }

    if(m_vao_id) {
        // fprintf(stdout, "[info] VAO: Deleted VAO, ID: %d\n", m_vao_id);
        glDeleteVertexArrays(1, &m_vao_id);
        m_vao_id = 0;
    }
}

bool VertexArray::has_been_created(void) const {
    return m_vao_id != 0;
}

void VertexArray::apply_vao_attributes(void) const {
    this->bind_vao();

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, m_vbo_id));

    for(int32_t att_index = 0; att_index < m_vbo_layout.m_next_attribute; ++att_index) {
        const BufferLayout::Attribute *att = &m_vbo_layout.m_attributes[att_index];
        const int32_t location = att_index;

        /* TODO glVertexAttribIPointer? */
        GL_CHECK(glEnableVertexAttribArray(location));
        GL_CHECK(glVertexAttribPointer(location, att->count, gl_from_buffer_data_type(att->data_type), GL_FALSE, m_vbo_layout.m_stride, (void *)((uint64_t)att->offset)));
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

uint32_t VertexArray::get_vao_id(void) const {
    return m_vao_id;
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

void BufferLayout::push_attribute(const char *name, int32_t count, BufferDataType data_type) {
    ASSERT(m_next_attribute < ARRAY_COUNT(m_attributes));
    const int32_t att_index = m_next_attribute;

    Attribute *att = &m_attributes[att_index];
    strcpy_s(att->name, ARRAY_COUNT(att->name), name); // @check
    att->count = count;
    att->offset = m_stride;
    att->data_type = data_type;

    m_stride += size_of_buffer_data_type(data_type) * count;
    m_combined_count += count;
    m_next_attribute += 1;
}
