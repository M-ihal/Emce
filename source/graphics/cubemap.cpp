#include "cubemap.h"

#include <glew.h>
#include <stb_image.h>

extern int32_t gl_wrap_from_texture_wrap(TextureWrap param);
extern int32_t gl_filter_from_texture_filter(TextureFilter param);

Cubemap::Cubemap(void) {
    m_cubemap_id = 0;
}

void Cubemap::delete_cubemap(void) {
    if(m_cubemap_id) {
        GL_CHECK(glDeleteTextures(1, &m_cubemap_id));
        m_cubemap_id = 0;
    }
}

bool Cubemap::load_from_file(const std::string (&filepaths)[6], bool flip_on_load) {
    fprintf(stdout, "[info] Cubemap: Loading from files, path: %s, and 5 others.\n", filepaths[0].c_str());

    GL_CHECK(glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_cubemap_id));
    if(!m_cubemap_id) {
        fprintf(stderr, "[error] Cubemap: Failed to create texture cubemap id.\n");
        return false;
    }
    
    GL_CHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
    GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemap_id));

    bool any_file_read_fail = false;

    stbi_set_flip_vertically_on_load(flip_on_load);
    for(int32_t index = 0; index < 6; ++index) {

        int32_t width;
        int32_t height;
        int32_t bpp;
        uint8_t *pixels = stbi_load(filepaths[index].c_str(), &width, &height, &bpp, 3);
        if(!pixels) {
            any_file_read_fail = true;
            break;
        }

        /* @note Cubemap restricted to RGB images */
        GL_CHECK(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + index, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels));

        free(pixels);
    }

    if(any_file_read_fail) {
        GL_CHECK(glDeleteTextures(1, &m_cubemap_id));
        fprintf(stderr, "[error] Cubemap: Failed to load one or more files.\n");
        return false;
    }

    this->set_filter_min(TextureFilter::LINEAR);
    this->set_filter_mag(TextureFilter::LINEAR);
    this->set_wrap_s(TextureWrap::CLAMP);
    this->set_wrap_t(TextureWrap::CLAMP);
    this->set_wrap_r(TextureWrap::CLAMP);

    // fprintf(stdout, "[info] Cubemap: Loaded successfully.\n");
    return true;
}

void Cubemap::bind_cubemap(void) {
    ASSERT(m_cubemap_id);
    GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemap_id));
}

void Cubemap::set_filter_min(TextureFilter param) {
    this->bind_cubemap();
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, gl_filter_from_texture_filter(param)));
    m_filter_min = param;
}

void Cubemap::set_filter_mag(TextureFilter param) {
    this->bind_cubemap();
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, gl_filter_from_texture_filter(param)));
    m_filter_mag = param;
}

void Cubemap::set_wrap_s(TextureWrap param) {
    this->bind_cubemap();
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, gl_wrap_from_texture_wrap(param)));
    m_wrap_s = param;
}

void Cubemap::set_wrap_t(TextureWrap param) {
    this->bind_cubemap();
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, gl_wrap_from_texture_wrap(param)));
    m_wrap_t = param;
}

void Cubemap::set_wrap_r(TextureWrap param) {
    this->bind_cubemap();
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, gl_wrap_from_texture_wrap(param)));
    m_wrap_r = param;
}
