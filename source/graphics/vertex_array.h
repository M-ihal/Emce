#pragma once

#include "common.h"

enum class BufferDataType : int32_t {
    INT   = 1,
    FLOAT = 2,
    UINT  = 3
};

enum class ArrayBufferUsage : int32_t {
    STATIC  = 1,
    DYNAMIC = 2
};

class BufferLayout {
    struct Attribute {
        char    name[32];
        int32_t count;
        int32_t offset;
        BufferDataType data_type;
    };

public:
    friend class VertexArray;

    BufferLayout(void);

    /* Reset attributes */
    void reset(void);

    /* Push next vertex attribute */
    void push_attribute(const char name[32], int32_t count, BufferDataType data_type);

private:
    int32_t m_stride;
    int32_t m_combined_count;
    int32_t m_next_attribute;
    Attribute m_attributes[8];
};

/* Unbind vertex array */
static void bind_no_vao(void);

class VertexArray {
public:
    CLASS_COPY_DISABLE(VertexArray);
    CLASS_MOVE_ALLOW(VertexArray);
    
    VertexArray(void) = default;

    /* Bind vertex array */
    void bind_vao(void) const;

    /* Creates array IDs, call only at init or after deletion */
    void create_vao(const BufferLayout &vbo_layout, ArrayBufferUsage usage);

    /* Delete IDs */
    void delete_vao(void);
    
    /* Check if vao has been created */
    bool has_been_created(void) const;

    /* Apply attrib pointers */
    void apply_vao_attributes(void) const;

    /* Recreate the buffers */
    void set_vbo_data(const void *data, size_t size);
    void set_ibo_data(const uint32_t *data, int32_t count);
    
    /* Upload data to existing buffers */
    void upload_vbo_data(const void *data, size_t size, int32_t offset) const;
    void upload_ibo_data(const uint32_t *data, int32_t count, int32_t offset) const;

    /* Debug */
    uint32_t get_vao_id(void) const;

    /* Get buffers' sizes */
    int32_t get_vbo_size(void) const;
    int32_t get_ibo_count(void) const;

private:
    uint32_t m_vao_id = 0;
    uint32_t m_vbo_id = 0;
    uint32_t m_ibo_id = 0;
    uint32_t m_vbo_size  = 0;
    uint32_t m_ibo_count = 0;
    BufferLayout m_vbo_layout;
    ArrayBufferUsage m_usage;
};
