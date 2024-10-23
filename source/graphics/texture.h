#pragma once

#include "common.h"
#include "math_types.h"

#include <string>

enum class TextureFilter : int8_t {
    LINEAR,
    NEAREST
};

enum class TextureWrap : int8_t {
    CLAMP,
    REPEAT
};

enum class TextureDataFormat : int8_t { 
    RED,
    RGB,
    RGBA,
    DEPTH24_STENCIL8,
    INVALID
};

enum class TextureDataType : int8_t {
    UNSIGNED_BYTE,
    INVALID
};

struct TextureLoadSpec {
    TextureDataFormat internal_format;
    TextureDataFormat data_format;
    TextureDataType   data_type;
};

class Texture {
public:
    CLASS_COPY_DISABLE(Texture);

    /* Framebuffer needs the id */
    friend class Framebuffer;

    /* Zeroes the variables, Does not generate texture ID */
    Texture(void);

    /* Need to be called in order to delete IDs */
    void delete_texture_if_exists(void);

    /* Load functions delete current texture ID if exists */
    bool load_empty(int32_t width, int32_t height, TextureDataFormat internal_format);
    bool load_from_memory(uint8_t *data, int32_t width, int32_t height, TextureLoadSpec spec);
    bool load_from_file(const std::string &filepath, bool flip_on_load = false, TextureLoadSpec spec = { TextureDataFormat::INVALID, TextureDataFormat::INVALID, TextureDataType::INVALID });

    void bind_texture(void) const;
    void bind_texture_unit(int32_t unit) const;

    void set_pixels(uint8_t *data, int32_t off_x, int32_t off_y, int32_t width, int32_t height, TextureDataFormat data_format, TextureDataType data_type);

    void set_filter_min(TextureFilter param);
    void set_filter_mag(TextureFilter param);

    void set_wrap_s(TextureWrap param);
    void set_wrap_t(TextureWrap param);

    vec2i get_size(void) const;

private:
    uint32_t m_texture_id;
    int32_t  m_width;
    int32_t  m_height;
    TextureDataFormat m_internal_format;

    TextureFilter m_filter_min;
    TextureFilter m_filter_mag;
    TextureWrap   m_wrap_s;
    TextureWrap   m_wrap_t;
};
