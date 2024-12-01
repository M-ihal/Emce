#include "texture_array.h"

#include <glew.h>

#include <stb_image.h>

extern int32_t gl_filter_from_texture_filter(TextureFilter param);
extern int32_t gl_wrap_from_texture_wrap(TextureWrap param);
extern int32_t gl_internal_format_from_texture_data_format(TextureDataFormat format);
extern int32_t gl_data_type_from_texture_data_type(TextureDataType type);
extern int32_t gl_data_format_from_texture_data_format(TextureDataFormat format);

bool TextureArray::load_empty_reserve(uint32_t width, uint32_t height, uint32_t count, TextureDataFormat internal_format) {
    ASSERT(m_tex_id == 0);

    GL_CHECK(glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_tex_id));
    if(m_tex_id == 0) {
        fprintf(stderr, "[Error] TextureArray: Failed to create 2D array texture.\n");
        return false;
    }

    int32_t gl_internal_format = gl_internal_format_from_texture_data_format(internal_format);

    GL_CHECK(glTextureStorage3D(m_tex_id, 1, gl_internal_format, width, height, count));

    m_width  = width;
    m_height = height;
    m_count  = count;

    this->set_filter_min(TextureFilter::NEAREST);
    this->set_filter_mag(TextureFilter::NEAREST);
    this->set_wrap_s(TextureWrap::CLAMP);
    this->set_wrap_t(TextureWrap::CLAMP);

    return true;
}

void TextureArray::delete_texture_array(void) {
    if(m_tex_id) {
        GL_CHECK(glDeleteTextures(1, &m_tex_id));
        m_tex_id = 0;
    }
}

void TextureArray::bind_texture(void) {
    GL_CHECK(glBindTexture(GL_TEXTURE_2D_ARRAY, m_tex_id));
}

void TextureArray::bind_texture_unit(uint32_t slot) {
    GL_CHECK(glBindTextureUnit(slot, m_tex_id));
}

void TextureArray::set_pixels(uint32_t slot, uint8_t *pixels, uint32_t width, uint32_t height, TextureDataFormat pixel_format, TextureDataType pixel_type, uint32_t off_x, uint32_t off_y) {
    ASSERT(m_tex_id);
    ASSERT(pixels);
    ASSERT((off_x + width)  <= m_width);
    ASSERT((off_y + height) <= m_height);
    ASSERT(slot < m_count);

    const int32_t gl_data_format = gl_data_format_from_texture_data_format(pixel_format);
    const int32_t gl_data_type   = gl_data_type_from_texture_data_type(pixel_type);

    GL_CHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
    GL_CHECK(glTextureSubImage3D(m_tex_id, 0, off_x, off_y, slot, width, height, 1, gl_data_format, gl_data_type, pixels));
}

void TextureArray::set_filter_min(TextureFilter param) {
    GL_CHECK(glTextureParameteri(m_tex_id, GL_TEXTURE_MIN_FILTER, gl_filter_from_texture_filter(param)));
    m_filter_min = param;
}

void TextureArray::set_filter_mag(TextureFilter param) {
    GL_CHECK(glTextureParameteri(m_tex_id, GL_TEXTURE_MAG_FILTER, gl_filter_from_texture_filter(param)));
    m_filter_mag = param;
}

void TextureArray::set_wrap_s(TextureWrap param) {
    GL_CHECK(glTextureParameteri(m_tex_id, GL_TEXTURE_WRAP_S, gl_wrap_from_texture_wrap(param)));
    m_wrap_s = param;
}

void TextureArray::set_wrap_t(TextureWrap param) {
    GL_CHECK(glTextureParameteri(m_tex_id, GL_TEXTURE_WRAP_T, gl_wrap_from_texture_wrap(param)));
    m_wrap_t = param;
}

uint32_t TextureArray::get_width(void) {
    return m_width;
}

uint32_t TextureArray::get_height(void) {
    return m_height;
}

uint32_t TextureArray::get_count(void) {
    return m_count;
}

uint8_t *copy_image_memory_rect(uint8_t *src, uint32_t src_w, uint32_t src_h, uint32_t src_channels,
                                uint32_t rect_x, uint32_t rect_y, uint32_t rect_w, uint32_t rect_h, uint8_t *dest_memory) {
    uint8_t *dest = NULL;

    if(dest_memory) {
        dest = dest_memory;
    } else {
        dest = (uint8_t *)malloc(rect_w * rect_h * src_channels * sizeof(uint8_t));
        ASSERT(dest);
    }

    const uint32_t stride_src = src_w * src_channels;
    const uint32_t stride_dest = rect_w * src_channels;

    for(int32_t cur_x = 0; cur_x < rect_w; ++cur_x) {
        for(int32_t cur_y = 0; cur_y < rect_h; ++cur_y) {
            for(int32_t cur_c = 0; cur_c < src_channels; ++cur_c) {
                dest[cur_y * stride_dest + cur_x * src_channels + cur_c] = src[(cur_y + rect_y) * stride_src + (cur_x + rect_x) * src_channels + cur_c];
            }
        }
    }

    return dest;
}
