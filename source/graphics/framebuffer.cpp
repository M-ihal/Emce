#include "framebuffer.h"
#include "simple_draw.h"

#include <glew.h>

Framebuffer::Framebuffer(void) {
    ZERO_STRUCT(*this);
}

/* Unbinds any framebuffer */
void Framebuffer::bind_no_fbo(void) {
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

/* Binds this framebuffer */
void Framebuffer::bind_fbo(void) {
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_id));
}

bool Framebuffer::create_fbo(int32_t width, int32_t height) {
    ASSERT(width && height);

    m_width  = width;
    m_height = height;

    if(!m_color.load_empty(width, height, TextureDataFormat::RGBA)) {
        fprintf(stderr, "[Error] Framebuffer: Failed to create color attachement.\n");
        return false;
    }

    if(!m_depth.load_empty(width, height, TextureDataFormat::DEPTH24_STENCIL8)) {
        fprintf(stderr, "[Error] Framebuffer: Failed to create depth attachement.\n");
        m_color.delete_texture_if_exists();
        return false;
    }
    
    m_color.set_filter_min(TextureFilter::LINEAR);
    m_color.set_filter_mag(TextureFilter::LINEAR);

    GL_CHECK(glGenFramebuffers(1, &m_fbo_id));
    if(!m_fbo_id) {
        fprintf(stderr, "[Error] Framebuffer: Failed to generate framebuffer id.\n");
        m_color.delete_texture_if_exists();
        m_depth.delete_texture_if_exists();
        return false;
    }

    this->bind_fbo();
    GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_color.m_texture_id, 0));
    GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_depth.m_texture_id, 0));
    
    GL_CHECK(bool is_complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    if(!is_complete) {
        fprintf(stderr, "[Error] Framebuffer: Framebuffer incomplete!\n");
        Framebuffer::bind_no_fbo();
        m_color.delete_texture_if_exists();
        m_depth.delete_texture_if_exists();
        GL_CHECK(glDeleteFramebuffers(1, &m_fbo_id));
        m_fbo_id = 0;
        return false;
    }

    return true;
}

void Framebuffer::delete_fbo(void) {
    m_color.delete_texture_if_exists();
    m_depth.delete_texture_if_exists();
    if(m_fbo_id) {
        GL_CHECK(glDeleteFramebuffers(1, &m_fbo_id));
        m_fbo_id = 0;
    }
}

/* @todo Can resize existing fbo? */
void Framebuffer::resize_fbo(int32_t new_width, int32_t new_height) {
    this->delete_fbo();
    this->create_fbo(new_width, new_height);
}

void Framebuffer::set_viewport(void) {
    GL_CHECK(glViewport(0, 0, this->m_width, this->m_height));
}

void Framebuffer::clear_fbo(const vec4 &color, uint32_t flags) {
    this->bind_fbo();
    this->set_viewport();
    GL_CHECK(glClearColor(color.r, color.g, color.b, color.a));

    uint32_t clear_flags = 0x0;
    if(flags & CLEAR_COLOR)   { clear_flags = clear_flags | GL_COLOR_BUFFER_BIT; }
    if(flags & CLEAR_DEPTH)   { clear_flags = clear_flags | GL_DEPTH_BUFFER_BIT; }
    if(flags & CLEAR_STENCIL) { clear_flags = clear_flags | GL_STENCIL_BUFFER_BIT; }
    GL_CHECK(glClear(clear_flags));
}

void Framebuffer::blit_whole(int32_t width, int32_t height) {
    GL_CHECK(glDisable(GL_DEPTH_TEST));
    GL_CHECK(glEnable(GL_BLEND));
    GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    mat4 proj_m = mat4::orthographic(0.0f, 0.0f, (float)width, (float)height, -10.0f, 10.0f);

    GL_CHECK(glViewport(0, 0, width, height));
    SimpleDraw::draw_textured_quad_2d({ 0.0f, 0.0f }, { (float)width, (float)height }, m_color, proj_m);
}

const Texture &Framebuffer::get_color_texture(void) const {
    return m_color;
}

const Texture &Framebuffer::get_depth_texture(void) const {
    return m_depth;
}

int32_t Framebuffer::get_width(void) const {
    return m_width;
}

int32_t Framebuffer::get_height(void) const {
    return m_height;
}

vec2i Framebuffer::get_size(void) const {
    return vec2i{ m_width, m_height };
}
