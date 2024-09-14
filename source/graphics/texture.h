#pragma once

#include "common.h"
#include "math_types.h"

enum class TextureFilter : int8_t {
    LINEAR,
    NEAREST
};

enum class TextureWrap : int8_t {
    CLAMP,
    REPEAT
};

enum class TextureDataFormat : int8_t { 
    RGB,
    RGBA,
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

    /* Zeroes the variables, Does not generate texture ID */
    Texture(void);

    /* Need to be called in order to delete IDs */
    void delete_texture_if_exists(void);

    /* Load functions delete current texture ID if exists */
    bool load_empty(int32_t width, int32_t height, TextureDataFormat internal_format);

    bool load_from_memory(uint8_t *data, int32_t width, int32_t height, TextureLoadSpec spec);

    bool load_from_file(const char *filepath, TextureLoadSpec spec = { TextureDataFormat::INVALID, TextureDataFormat::INVALID, TextureDataType::INVALID });

    void bind_texture(void);

    void bind_texture_unit(int32_t unit);

    void set_pixels(uint8_t *data, int32_t off_x, int32_t off_y, int32_t width, int32_t height, TextureDataFormat data_format, TextureDataType data_type);

    void set_filter_min(int32_t filter);

    void set_filter_mag(int32_t filter);

    void set_wrap_s(int32_t wrap);

    void set_wrap_t(int32_t wrap);

    vec2i get_size(void);

private:
    uint32_t m_texture_id;
    int32_t  m_width;
    int32_t  m_height;
    TextureDataFormat m_internal_format;

    int32_t  m_filter_min;
    int32_t  m_filter_mag;
    int32_t  m_wrap_s;
    int32_t  m_wrap_t;
};
