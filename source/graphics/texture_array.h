#pragma once

#include "common.h"
#include "math.h"
#include "texture.h"

class TextureArray {
public:
    CLASS_COPY_DISABLE(TextureArray); 

    TextureArray(void) = default;

    /* Must be called once or after deletion */
    /* Generates Texture 2d array for *count* textures */
    bool load_empty_reserve(uint32_t width, uint32_t height, uint32_t count, TextureDataFormat internal_format);

    /* Deletes the texture */
    void delete_texture_array(void);

    /* Bind texture array */
    void bind_texture(void);

    /* Bind texture array to specific slot */
    void bind_texture_unit(uint32_t unit);

    /* Sets pixels for *slot* texture in the array */
    void set_pixels(uint32_t slot, char *pixels, uint32_t width, uint32_t height, TextureDataFormat pixel_format, TextureDataType pixel_type);

    /* Sets the Texture min filter */
    void set_filter_min(TextureFilter param);

    /* Sets the Texture mag filter */
    void set_filter_mag(TextureFilter param);

    /* Sets the Texture wrap s mode */
    void set_wrap_s(TextureWrap param);

    /* Sets the Texture wrap t mode */
    void set_wrap_t(TextureWrap param);

private:
    uint32_t m_tex_id = 0;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_count;
    TextureDataFormat m_internal_format;

    TextureFilter m_filter_min;
    TextureFilter m_filter_mag;
    TextureWrap   m_wrap_s;
    TextureWrap   m_wrap_t;
};
