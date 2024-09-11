#pragma once
#include "common.h"

#define VA_VB_MAX 8

enum class ArrayBufferDataType : int32_t {
    INT   = 1,
    FLOAT = 2,
};

// @todo : Make data type be opengl independant
class BufferLayout {
private:
    struct Attribute {
        char    name[32];
        int32_t count;
        int32_t offset;
        int32_t data_type;
        int32_t data_type_size;
    };

public:
    friend class VertexArray;

    BufferLayout(void);

    void reset(void);

    /* name max length : 32 */
    void push_attribute(const char *name, int32_t count, int32_t data_type, int32_t data_type_size);

private:
    int32_t m_stride;
    int32_t m_combined_count;
    int32_t m_next_attribute;
    Attribute m_attributes[8];
};

enum class ArrayBufferUsage : int32_t {
    STATIC  = 1,
    DYNAMIC = 2
};

class VertexArray {
public:
    CLASS_COPY_DISABLE(VertexArray);
    CLASS_MOVE_ALLOW(VertexArray);

    /*
        Constructors
    */

    VertexArray(void);

    ~VertexArray(void);

    static void bind_no_vao(void);

    void bind_vao(void);

    /* Set attribute pointers from attached vertex buffers */
    void apply_vertex_attributes(void);

    /*
        Buffers
    */

    void add_vertex_buffer(const void *data, size_t data_size, ArrayBufferUsage usage, const BufferLayout &layout);

    void add_index_buffer(const uint32_t *index_data, size_t index_count, ArrayBufferUsage usage);

    uint32_t index_count(void) const;
private:
    uint32_t m_vao_id;

    /* Vbos */
    uint32_t     m_vbo_count;
    uint32_t     m_vbo_id[VA_VB_MAX];
    BufferLayout m_vbo_layout[VA_VB_MAX];

    /* Ibo */
    uint32_t m_ibo_id;
    uint32_t m_ibo_index_count;
};
