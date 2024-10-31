#include "texture_array.h"

#include <glew.h>

extern constexpr int32_t gl_filter_from_texture_filter(TextureFilter param);
extern constexpr int32_t gl_wrap_from_texture_wrap(TextureWrap param);
extern constexpr int32_t gl_internal_format_from_texture_data_format(TextureDataFormat format);
extern constexpr int32_t gl_data_type_from_texture_data_type(TextureDataType type);
extern constexpr int32_t gl_data_format_from_texture_data_format(TextureDataFormat format);

bool TextureArray::load_empty_reserve(const char *filepath, int32_t width, int32_t height, int32_t count, TextureDataFormat internal_format) {
    ASSERT(m_tex_id == 0);

    GL_CHECK(glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_tex_id));
    if(m_tex_id == 0) {
        fprintf(stderr, "[Error] TextureArray: Failed to create 2D array texture.\n");
        return false;
    }

    int32_t gl_internal_format = gl_internal_format_from_texture_data_format(internal_format);

    GL_CHECK(glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, gl_internal_format, width, height, count));

//    this->set_filter_min(TextureFilter::NEAREST);
//    this->set_filter_mag(TextureFilter::NEAREST);
//    this->set_wrap_s(TextureWrap::CLAMP);
//    this->set_wrap_t(TextureWrap::CLAMP);

    return true;
}

void TextureArray::delete_texture_array(void) {
    if(m_tex_id) {
        INVALID_CODE_PATH;
    }
}

void TextureArray::set_pixels(int32_t slot, char *pixels, int32_t width, int32_t height, TextureDataFormat pixel_format, TextureDataType pixel_type) {
    ASSERT(m_tex_id);
}
