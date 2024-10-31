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
    bool load_empty_reserve(const char *filepath, int32_t width, int32_t height, int32_t count, TextureDataFormat internal_format);

    /* Deletes the texture */
    void delete_texture_array(void);

    /* Sets pixels for *slot* texture in the array */
    void set_pixels(int32_t slot, char *pixels, int32_t width, int32_t height, TextureDataFormat pixel_format, TextureDataType pixel_type);

    /* Sets the Texture min filter */
    void set_filter_min(TextureFilter param);

    /* Sets the Texture mag filter */
    void set_filter_mag(TextureFilter param);

    /* Sets the Texture wrap s mode */
    void set_wrap_s(TextureWrap param);

    /* Sets the Texture wrap t mode */
    void set_wrap_t(TextureWrap param);

    /* Sets the Texture wrap r mode */
    // void set_wrap_r(TextureWrap param);

private:
    uint32_t m_tex_id = 0;
    int32_t  m_width;
    int32_t  m_height;
    int32_t  m_count;
    TextureDataFormat m_internal_format;

    TextureFilter m_filter_min;
    TextureFilter m_filter_mag;
    TextureWrap   m_wrap_s;
    TextureWrap   m_wrap_t;
    // TextureWrap   m_wrap_r;
};
