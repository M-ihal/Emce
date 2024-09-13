#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glew.h>

bool Texture::init_empty_texture(int32_t width, int32_t height, int32_t bpp, int32_t internal_format) {
    ZERO_STRUCT(*this);

    uint32_t texture_id = 0;
    GL_CHECK(glCreateTextures(GL_TEXTURE_2D, 1, &texture_id));
    if(!texture_id) {
        fprintf(stderr, "[error] Texture: Failed to create texture id.\n");
        return false;
    }

    GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture_id));
    GL_CHECK(glTexStorage2D(GL_TEXTURE_2D, 1, internal_format, width, height));

    m_texture_id      = texture_id;
    m_width           = width;
    m_height          = height;
    m_bpp             = bpp;
    m_internal_format = internal_format;

    this->set_filter_min(GL_LINEAR);
    this->set_filter_mag(GL_LINEAR);
    this->set_wrap_s(GL_CLAMP_TO_EDGE);
    this->set_wrap_t(GL_CLAMP_TO_EDGE);

    // ASSERT some stuff here maybe @TODO

    fprintf(stdout, "[info] Texture: Created texture, ID: %d (%p)\n", m_texture_id, this);
    
    return true;
}

Texture::Texture(uint8_t *pixels, int32_t width, int32_t height, int32_t bpp, int32_t internal_format, int32_t data_format, int32_t data_type) {
    bool success = this->init_empty_texture(width, height, bpp, internal_format);
    if(!success) {
        return;
    }

    this->set_pixels(pixels, width, height, 0, 0, data_format, data_type);
}

Texture::Texture(const char *filepath) {
    fprintf(stdout, "[info] Texture: Loading texture from file, path: %s\n", filepath);

    int32_t width;
    int32_t height;
    int32_t bpp;
    uint8_t *pixels = stbi_load(filepath, &width, &height, &bpp, 0);
    if(!pixels) {
        fprintf(stderr, "[error] Texture: Failed to read image file: \"%s\"\n", filepath);
        return;
    }

    const int32_t internal_format = GL_RGBA8; // Always use 4 channels?

    bool success = this->init_empty_texture(width, height, bpp, internal_format);
    if(!success) {
        free(pixels);
        return;
    }

    const int32_t data_type   = GL_UNSIGNED_BYTE;
    const int32_t data_format = bpp == 3 ? GL_RGB : GL_RGBA;

    this->set_pixels(pixels, width, height, 0, 0, data_format, data_type);
    free(pixels);
}

Texture::~Texture(void) {
    if(m_texture_id) {
        GL_CHECK(glDeleteTextures(1, &m_texture_id));
        m_texture_id = 0;
    }
}

void Texture::bind_texture(void) {
    ASSERT(m_texture_id);
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, m_texture_id));
}

void Texture::bind_texture_unit(int32_t unit) {
    ASSERT(m_texture_id);
    GL_CHECK(glBindTextureUnit(unit, m_texture_id));
}

vec2i Texture::size(void) {
    return vec2i{ m_width, m_height };
}

void Texture::set_pixels(uint8_t *pixels, int32_t width, int32_t height, int32_t offset_x, int32_t offset_y, int32_t data_format, int32_t data_type) {
    ASSERT(pixels);
    ASSERT(width && height);
    ASSERT((offset_x + width) <= m_width);
    ASSERT((offset_y + height) <= m_height);

    this->bind_texture();
    GL_CHECK(glTexSubImage2D(GL_TEXTURE_2D, 0, offset_x, offset_y, width, height, data_format, data_type, pixels));
}

void Texture::set_filter_min(int32_t param) {
    this->bind_texture();
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, param));
    m_filter_min = param;
}

void Texture::set_filter_mag(int32_t param) {
    this->bind_texture();
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, param));
    m_filter_mag = param;
}

void Texture::set_wrap_s(int32_t param) {
    this->bind_texture();
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, param));
    m_wrap_s = param;
}

void Texture::set_wrap_t(int32_t param) {
    this->bind_texture();
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, param));
    m_wrap_t = param;
}
