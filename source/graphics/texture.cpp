#include "texture.h"

#include <glew.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

int32_t gl_filter_from_texture_filter(TextureFilter param);
int32_t gl_wrap_from_texture_wrap(TextureWrap param);
int32_t gl_internal_format_from_texture_data_format(TextureDataFormat format);
int32_t gl_data_format_from_texture_data_format(TextureDataFormat format);
int32_t gl_data_type_from_texture_data_type(TextureDataType type);

void Texture::delete_texture(void) {
    if(m_tex_id) {
        GL_CHECK(glDeleteTextures(1, &m_tex_id));
        m_tex_id = 0;
    }
}

bool Texture::load_empty(int32_t width, int32_t height, TextureDataFormat internal_format, int32_t levels) {
    ASSERT(m_tex_id == 0);

    GL_CHECK(glCreateTextures(GL_TEXTURE_2D, 1, &m_tex_id));
    if(!m_tex_id) {
        fprintf(stderr, "[error] Texture: Failed to create texture id.\n");
        return false;
    }

    const int32_t gl_internal_format = gl_internal_format_from_texture_data_format(internal_format);

    GL_CHECK(glBindTexture(GL_TEXTURE_2D, m_tex_id));
    GL_CHECK(glTexStorage2D(GL_TEXTURE_2D, levels, gl_internal_format, width, height));

    m_width = width;
    m_height = height;
    m_levels = levels;
    m_internal_format = internal_format;

    this->set_filter_min(TextureFilter::NEAREST);
    this->set_filter_mag(TextureFilter::NEAREST);
    this->set_wrap_s(TextureWrap::CLAMP);
    this->set_wrap_t(TextureWrap::CLAMP);

    return true;
}

bool Texture::load_from_memory(uint8_t *data, int32_t width, int32_t height, TextureLoadSpec spec, int32_t levels) {
    ASSERT(data);
    ASSERT(width && height);

    if(!this->load_empty(width, height, spec.internal_format, levels)) {
        return false;
    }

    this->set_pixels(data, 0, 0, width, height, spec.data_format, spec.data_type);
    this->gen_mipmaps();

    return true;
}

bool Texture::load_from_file(const std::string &filepath, bool flip_on_load, TextureLoadSpec spec, int32_t levels) {
    fprintf(stdout, "[info] Texture: Loading texture from file, path: %s\n", filepath.c_str());

    stbi_set_flip_vertically_on_load(flip_on_load);

    int32_t width;
    int32_t height;
    int32_t bpp;
    uint8_t *pixels = stbi_load(filepath.c_str(), &width, &height, &bpp, 0);

    if(!pixels) {
        fprintf(stderr, "[error] Texture: Failed to read texture file\n");
        return false;
    }

    /* If passed INVALID -> assume the format is RGBA or RGB  */
    TextureDataFormat internal_format;
    if(spec.internal_format == TextureDataFormat::INVALID) {
        switch(bpp) {
            default: INVALID_CODE_PATH; break;
            case 3: internal_format = TextureDataFormat::RGB;  break;
            case 4: internal_format = TextureDataFormat::RGBA; break;
        }
    } else {
        internal_format = spec.internal_format;
    }

    if(!this->load_empty(width, height, internal_format, levels)) {
        free(pixels);
        return false;
    }

    /* If passed INVALID for these -> assume the format is RGBA or RGB, and the type is UNSIGNED_BYTE */
    TextureDataType data_type;
    if(spec.data_type == TextureDataType::INVALID) {
        data_type = TextureDataType::UNSIGNED_BYTE;
    } else {
        data_type = spec.data_type;
    }

    TextureDataFormat data_format;
    if(spec.data_format == TextureDataFormat::INVALID) {
        switch(bpp) {
            default: INVALID_CODE_PATH; break;
            case 3: data_format = TextureDataFormat::RGB;  break;
            case 4: data_format = TextureDataFormat::RGBA; break;
        }
    } else {
        data_format = spec.data_format;
    }

    this->set_pixels(pixels, 0, 0, width, height, data_format, data_type);
    this->gen_mipmaps();
    free(pixels);

    return true;
}

void Texture::bind_texture(void) const {
    ASSERT(m_tex_id);
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, m_tex_id));
}

void Texture::bind_texture_unit(uint32_t unit) const {
    ASSERT(m_tex_id);
    GL_CHECK(glBindTextureUnit(unit, m_tex_id));
}

void Texture::set_pixels(uint8_t *data, int32_t off_x, int32_t off_y, int32_t width, int32_t height, TextureDataFormat data_format, TextureDataType data_type) {
    ASSERT(m_tex_id);
    ASSERT(data);
    ASSERT(width && height);
    ASSERT((off_x + width)  <= m_width);
    ASSERT((off_y + height) <= m_height);

    const int32_t gl_data_format = gl_data_format_from_texture_data_format(data_format);
    const int32_t gl_data_type   = gl_data_type_from_texture_data_type(data_type);

    // Keep this?
    GL_CHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

    this->bind_texture();
    GL_CHECK(glTexSubImage2D(GL_TEXTURE_2D, 0, off_x, off_y, width, height, gl_data_format, gl_data_type, data));

    // Regenerate mipmaps @TODO move?
    GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D));
}

void Texture::set_filter_min(TextureFilter param) {
    this->bind_texture();
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_from_texture_filter(param)));
    m_filter_min = param;
}

void Texture::set_filter_mag(TextureFilter param) {
    this->bind_texture();
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_from_texture_filter(param)));
    m_filter_mag = param;
}

void Texture::set_wrap_s(TextureWrap param) {
    this->bind_texture();
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl_wrap_from_texture_wrap(param)));
    m_wrap_s = param;
}

void Texture::set_wrap_t(TextureWrap param) {
    this->bind_texture();
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl_wrap_from_texture_wrap(param)));
    m_wrap_t = param;
}

void Texture::gen_mipmaps(void) {
    if(m_levels > 1) {
        GL_CHECK(glGenerateTextureMipmap(m_tex_id));
    }
}

vec2i Texture::get_size(void) const {
    return vec2i{ m_width, m_height };
}

int32_t gl_filter_from_texture_filter(TextureFilter param) {
    switch(param) {
        default: INVALID_CODE_PATH;  return -1;
        case TextureFilter::NEAREST: return GL_NEAREST;
        case TextureFilter::LINEAR:  return GL_LINEAR;
        case TextureFilter::LINEAR_MIPMAP_LINEAR: return GL_LINEAR_MIPMAP_LINEAR;
        case TextureFilter::LINEAR_MIPMAP_NEAREST: return GL_LINEAR_MIPMAP_NEAREST;
        case TextureFilter::NEAREST_MIPMAP_LINEAR: return GL_NEAREST_MIPMAP_LINEAR;
        case TextureFilter::NEAREST_MIPMAP_NEAREST: return GL_NEAREST_MIPMAP_NEAREST;
    }
}

int32_t gl_wrap_from_texture_wrap(TextureWrap param) {
    switch(param) {
        default: INVALID_CODE_PATH; return -1;
        case TextureWrap::CLAMP:    return GL_CLAMP_TO_EDGE;
        case TextureWrap::REPEAT:   return GL_REPEAT;
    }
}

int32_t gl_internal_format_from_texture_data_format(TextureDataFormat format) {
    switch(format) {
        default: INVALID_CODE_PATH;   return -1;
        case TextureDataFormat::RED:  return GL_R8;
        case TextureDataFormat::RGB:  return GL_RGB8;
        case TextureDataFormat::RGBA: return GL_RGBA8;
        case TextureDataFormat::DEPTH24_STENCIL8: return GL_DEPTH24_STENCIL8;
    }
}

int32_t gl_data_format_from_texture_data_format(TextureDataFormat format) {
    switch(format) {
        default: INVALID_CODE_PATH;   return -1;
        case TextureDataFormat::RED:  return GL_RED;
        case TextureDataFormat::RGB:  return GL_RGB;
        case TextureDataFormat::RGBA: return GL_RGBA;
        case TextureDataFormat::DEPTH24_STENCIL8: return GL_DEPTH24_STENCIL8;
    }
}

int32_t gl_data_type_from_texture_data_type(TextureDataType type) {
    switch(type) {
        default: INVALID_CODE_PATH;          return -1;
        case TextureDataType::UNSIGNED_BYTE: return GL_UNSIGNED_BYTE;
    }
}
