#include "texture_array.h"

#include <glew.h>

extern constexpr int32_t gl_filter_from_texture_filter(TextureFilter param);
extern constexpr int32_t gl_wrap_from_texture_wrap(TextureWrap param);
extern constexpr int32_t gl_internal_format_from_texture_data_format(TextureDataFormat format);
extern constexpr int32_t gl_data_type_from_texture_data_type(TextureDataType type);
extern constexpr int32_t gl_data_format_from_texture_data_format(TextureDataFormat format);

bool TextureArray::load_empty_reserve(uint32_t width, uint32_t height, uint32_t count, TextureDataFormat internal_format) {
    ASSERT(m_tex_id == 0);

    GL_CHECK(glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_tex_id));
    if(m_tex_id == 0) {
        fprintf(stderr, "[Error] TextureArray: Failed to create 2D array texture.\n");
        return false;
    }

    int32_t gl_internal_format = gl_internal_format_from_texture_data_format(internal_format);

    GL_CHECK(glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, gl_internal_format, width, height, count));

    this->set_filter_min(TextureFilter::NEAREST);
    this->set_filter_mag(TextureFilter::NEAREST);
    this->set_wrap_s(TextureWrap::CLAMP);
    this->set_wrap_t(TextureWrap::CLAMP);

    return true;
}

void TextureArray::delete_texture_array(void) {
    if(m_tex_id) {
        INVALID_CODE_PATH;
    }
}

void TextureArray::bind_texture(void) {
    GL_CHECK(glBindTexture(GL_TEXTURE_2D_ARRAY, m_tex_id));
}

void TextureArray::bind_texture_unit(uint32_t slot) {
    GL_CHECK(glBindTextureUnit(slot, m_tex_id));
}

void TextureArray::set_pixels(uint32_t slot, char *pixels, uint32_t width, uint32_t height, TextureDataFormat pixel_format, TextureDataType pixel_type) {
    ASSERT(m_tex_id);
    ASSERT(slot < m_count);
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

