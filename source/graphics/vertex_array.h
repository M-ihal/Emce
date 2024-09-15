#pragma once
#include "common.h"

// #define DO_VAO_LOGS

enum class BufferDataType : int32_t {
    INT   = 1,
    FLOAT = 2,
};

class BufferLayout {
private:
    struct Attribute {
        char    name[32];
        int32_t count;
        int32_t offset;
        BufferDataType data_type;
    };

public:
    friend class VertexArray;

    BufferLayout(void);

    void reset(void);

    /* name max length : 32 */
    void push_attribute(const char *name, int32_t count, BufferDataType data_type);

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

    /* Initialize variables */
    VertexArray(void);

    /* Unbind any bound vertex array */
    static void bind_no_vao(void);

    /* Bind vertex array */
    void bind_vao(void) const;

    /* Create or recreates IDs */
    void create_vao(const BufferLayout &vbo_layout, ArrayBufferUsage usage);

    /* Delete IDs */
    void delete_vao(void);
    
    /* Apply Attrib Pointers */
    void apply_vao_attributes(void) const;

    /* Recreate the buffers */
    void set_vbo_data(const void *data, size_t size);
    void set_ibo_data(const uint32_t *data, int32_t count);
    
    /* Upload data to existing buffers */
    void upload_vbo_data(const void *data, size_t size, int32_t offset) const;
    void upload_ibo_data(const uint32_t *data, int32_t count, int32_t offset) const;

    /* Get buffers' sizes */
    int32_t get_vbo_size(void) const;
    int32_t get_ibo_count(void) const;

private:
    void delete_vao_if_exists(void);

    uint32_t m_vao_id;
    uint32_t m_vbo_id;
    uint32_t m_ibo_id;
    uint32_t m_vbo_size;
    uint32_t m_ibo_count;
    BufferLayout m_vbo_layout;
    ArrayBufferUsage m_usage;
};
