#include "vertex_array.h"

#include <glew.h>
#include <stdio.h>
#include <cstring>

/*
    VertexArray

    TODO
        - Better logs.....
*/

namespace {
}

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
        default: {
            INVALID_CODE_PATH;
            return -1;
        }
        case ArrayBufferDataType::INT:
        case ArrayBufferDataType::FLOAT: {
            return 4;
        }
    }
}

VertexArray::VertexArray(void) {
    m_vao_id = 0;
    m_vbo_count = 0;
    ZERO_ARRAY(m_vbo_id);
    ZERO_ARRAY(m_vbo_layout);
    m_ibo_id = 0;
    m_ibo_index_count = 0;

    GL_CHECK(glGenVertexArrays(1, &m_vao_id));
    if(!m_vao_id) {
        fprintf(stderr, "[error] VAO: Failed to generate vertex array id.\n");
        return;
    }

    VertexArray::bind_no_vao();

    fprintf(stdout, "[info] VAO: Created vertex array, ID: %d (%p)\n", m_vao_id, this);
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

    const int32_t vbo_idx = m_vbo_count++;
    m_vbo_id[vbo_idx] = vbo_id;
    m_vbo_layout[vbo_idx] = layout;

    fprintf(stdout, "[info] VAO: VBO ID: %d attached to VAO ID: %d (%p)\n", vbo_id, m_vao_id, this);
    VertexArray::bind_no_vao();
}

void VertexArray::add_index_buffer(uint32_t *index_data, size_t index_count, ArrayBufferUsage usage) {
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
