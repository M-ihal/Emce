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
    void set_pixels(uint32_t slot, uint8_t *pixels, uint32_t width, uint32_t height, TextureDataFormat pixel_format, TextureDataType pixel_type, uint32_t off_x = 0, uint32_t off_y = 0);

    /* Sets the Texture min filter */
    void set_filter_min(TextureFilter param);

    /* Sets the Texture mag filter */
    void set_filter_mag(TextureFilter param);

    /* Sets the Texture wrap s mode */
    void set_wrap_s(TextureWrap param);

    /* Sets the Texture wrap t mode */
    void set_wrap_t(TextureWrap param);

    /* Getters */
    uint32_t get_width(void);

    uint32_t get_height(void);

    uint32_t get_count(void);

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

/* optional dest_memory must be of correct size, if not passed, the resulting memory gets malloc'ed */
uint8_t *copy_image_memory_rect(uint8_t *src, uint32_t src_w, uint32_t src_h, uint32_t src_channels,
                                uint32_t rect_x, uint32_t rect_y, uint32_t rect_w, uint32_t rect_h, uint8_t *dest_memory = NULL);

