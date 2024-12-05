#pragma once

#include "common.h"
#include "math_types.h"

#include <string>

enum class TextureFilter : int8_t {
    LINEAR  = 1,
    NEAREST = 2,
    LINEAR_MIPMAP_LINEAR = 3,
    LINEAR_MIPMAP_NEAREST = 4,
    NEAREST_MIPMAP_LINEAR = 5,
    NEAREST_MIPMAP_NEAREST = 6,
};

enum class TextureWrap : int8_t {
    CLAMP  = 1,
    REPEAT = 2
};

enum class TextureDataFormat : int8_t { 
    RED  = 1,
    RGB  = 4,
    RGBA = 5,
    DEPTH24_STENCIL8 = 12,
    INVALID = 124
};

enum class TextureDataType : int8_t {
    UNSIGNED_BYTE = 1,
    INVALID = 126
};

struct TextureLoadSpec {
    TextureDataFormat internal_format;
    TextureDataFormat data_format;
    TextureDataType   data_type;
};

class Texture {
public:
    CLASS_COPY_DISABLE(Texture);

    Texture(void) = default;

    /* Binds texture 2d */
    void bind_texture(void) const;

    /* Binds texture 2d on given unit */
    void bind_texture_unit(uint32_t unit) const;

    /* load_* procs should be called only once or after Texture deletion */

    /* Generates empty texture */
    bool load_empty(int32_t width, int32_t height, TextureDataFormat internal_format, int32_t levels = 1);

    /* Generates texture and sets the pixels from memory */
    bool load_from_memory(uint8_t *data, int32_t width, int32_t height, TextureLoadSpec spec, int32_t levels = 1);

    /* Generates texture from file */
    bool load_from_file(const std::string &filepath, bool flip_on_load = false, TextureLoadSpec spec = { TextureDataFormat::INVALID, TextureDataFormat::INVALID, TextureDataType::INVALID }, int32_t levels = 1);

    /* Deletes the texture */
    void delete_texture(void);

    /* Sets the pixels */
    void set_pixels(uint8_t *data, int32_t off_x, int32_t off_y, int32_t width, int32_t height, TextureDataFormat data_format, TextureDataType data_type);

    /* Sets the Texture min filter */
    void set_filter_min(TextureFilter param);

    /* Sets the Texture mag filter */
    void set_filter_mag(TextureFilter param);

    /* Sets the Texture wrap s mode */
    void set_wrap_s(TextureWrap param);

    /* Sets the Texture wrap t mode */
    void set_wrap_t(TextureWrap param);

    /* Generate mipmaps */
    void gen_mipmaps(void);

    /* */
    vec2i get_size(void) const;

private:
    uint32_t m_tex_id = 0;
    int32_t  m_width;
    int32_t  m_height;
    int32_t  m_levels;
    TextureDataFormat m_internal_format;

    TextureFilter m_filter_min;
    TextureFilter m_filter_mag;
    TextureWrap   m_wrap_s;
    TextureWrap   m_wrap_t;
};
