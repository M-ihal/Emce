#include "framebuffer.h"

#include <glew.h>

extern int32_t gl_filter_from_texture_filter(TextureFilter param);
extern int32_t gl_wrap_from_texture_wrap(TextureWrap param);
extern int32_t gl_internal_format_from_texture_data_format(TextureDataFormat format);
extern int32_t gl_data_format_from_texture_data_format(TextureDataFormat format);
extern int32_t gl_data_type_from_texture_data_type(TextureDataType type);

void bind_no_fbo(void) {
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void Framebuffer::bind_fbo(void) {
    ASSERT(m_fbo_id);
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_id));
}

bool Framebuffer::create_fbo(int32_t width, int32_t height, FboConfig &config) {
    ASSERT(m_fbo_id == 0);
    ASSERT(width && height);

    m_width  = width;
    m_height = height;
    m_config = config;

    GL_CHECK(glGenFramebuffers(1, &m_fbo_id));
    if(!m_fbo_id) {
        fprintf(stderr, "[Error] Framebuffer: Failed to generate framebuffer id.\n");
        return false;
    }

    this->gen_attachments();

    int32_t fbo_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(fbo_status != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "[Error] Framebuffer: Framebuffer incomplete!\n");

        this->delete_fbo();
        return false;
    }

    return true;
}

void Framebuffer::delete_fbo(void) {
    if(m_fbo_id) {
        this->delete_attachments();

        GL_CHECK(glDeleteFramebuffers(1, &m_fbo_id));
        m_fbo_id = 0;
    }
}

void Framebuffer::resize_fbo(int32_t width, int32_t height) {
    FboConfig config = m_config;

    this->delete_fbo();
    this->create_fbo(width, height, config);
}

void Framebuffer::clear_all(const vec4 &color, float depth_value, int32_t stencil_value) {
    for(uint32_t slot = 0; slot < m_config.color_attachment_count; ++slot) {
        this->clear_color(slot, color);
    }
    this->clear_depth(depth_value, stencil_value);
}

void Framebuffer::clear_color(uint32_t slot, const vec4 &color) {
    if(slot >= m_config.color_attachment_count) {
        return;
    }
    GL_CHECK(glClearNamedFramebufferfv(m_fbo_id, GL_COLOR, slot, color.e));
}

void Framebuffer::clear_depth(float depth_value, int32_t stencil_value) {
    if(!m_config.depth_attachment_set) {
        return;
    }
    GL_CHECK(glClearNamedFramebufferfi(m_fbo_id, GL_DEPTH_STENCIL, 0, depth_value, stencil_value));
}

void Framebuffer::set_draw_buffers(void) {
    /* Specify color buffers to be drawn into */
    if(m_config.color_attachment_count) {
        const GLenum gl_buffers[FBO_COLOR_MAX] = { 
            GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, 
            GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, 
            GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, 
            GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7 
        };

        GL_CHECK(glDrawBuffers(m_config.color_attachment_count, gl_buffers));
    } else {
        GL_CHECK(glDrawBuffer(GL_NONE));
    }
}

uint32_t Framebuffer::get_fbo_id(void) {
    return m_fbo_id;
}

uint32_t Framebuffer::get_color_attachment_id(uint32_t slot) {
    ASSERT(slot < m_config.color_attachment_count);
    return m_color_attachments[slot];
}

uint32_t Framebuffer::get_depth_attachment_id(void) {
    ASSERT(m_config.depth_attachment_set);
    return m_depth_attachment;
}

void Framebuffer::bind_color_texture_unit(uint32_t unit, uint32_t slot) {
    GL_CHECK(glBindTextureUnit(unit, this->get_color_attachment_id(slot)));
}

void Framebuffer::blit_color_attachment(uint32_t slot, Framebuffer &dest, uint32_t dest_slot) {
    GL_CHECK(glNamedFramebufferReadBuffer(m_fbo_id, GL_COLOR_ATTACHMENT0 + slot));
    GL_CHECK(glNamedFramebufferDrawBuffer(dest.get_fbo_id(), GL_COLOR_ATTACHMENT0 + dest_slot));
    GL_CHECK(glBlitNamedFramebuffer(m_fbo_id, dest.get_fbo_id(), 0, 0, m_width, m_height, 0, 0, dest.get_width(), dest.get_height(), GL_COLOR_BUFFER_BIT, GL_LINEAR));
}

void Framebuffer::blit_color_attachment(uint32_t slot, int32_t width, int32_t height) {
    GL_CHECK(glNamedFramebufferReadBuffer(m_fbo_id, GL_COLOR_ATTACHMENT0 + slot));
    GL_CHECK(glBlitNamedFramebuffer(m_fbo_id, 0, 0, 0, m_width, m_height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST));
}

void Framebuffer::blit_depth_attachment(Framebuffer &dest) {
    GL_CHECK(glBlitNamedFramebuffer(m_fbo_id, dest.get_fbo_id(), 0, 0, m_width, m_height, 0, 0, dest.get_width(), dest.get_height(), GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST));
}

void Framebuffer::blit_depth_attachment(int32_t width, int32_t height) {
    GL_CHECK(glBlitNamedFramebuffer(m_fbo_id, 0, 0, 0, m_width, m_height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST));
}

bool Framebuffer::is_complete(void) {
    return m_fbo_id != 0 && m_attachments_generated;
}

int32_t Framebuffer::get_width(void) {
    return m_width;
}

int32_t Framebuffer::get_height(void) {
    return m_height;
}

vec2i Framebuffer::get_size(void) {
    return vec2i{ m_width, m_height };
}

void Framebuffer::gen_attachments(void) {
    ASSERT(m_config.color_attachment_count <= FBO_COLOR_MAX);

    this->bind_fbo();

    /* Generate color attachments */
    for(uint32_t slot = 0; slot < m_config.color_attachment_count; ++slot) {
        FboColorConfig color = m_config.color_attachments[slot];
        
        int32_t gl_target = 0; /* Set in the switch statement */
        int32_t gl_format = gl_internal_format_from_texture_data_format(color.format);

        uint32_t attachment_id = 0;
        switch(color.target) {
            default: { ASSERT(false); fprintf(stderr, "[Error] Framebuffer: Invalid color attachment target!\n"); } return;

            case FboAttachmentTarget::TEXTURE: {
                gl_target = GL_TEXTURE_2D;

                GL_CHECK(glCreateTextures(GL_TEXTURE_2D, 1, &attachment_id));
                GL_CHECK(glBindTexture(GL_TEXTURE_2D, attachment_id));
                GL_CHECK(glTexStorage2D(GL_TEXTURE_2D, 1, gl_format, m_width, m_height));

                GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
                GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
                GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
                GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
                GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
            } break;

            case FboAttachmentTarget::TEXTURE_MULTISAMPLE: {
                gl_target = GL_TEXTURE_2D_MULTISAMPLE;

                GL_CHECK(glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &attachment_id));
                GL_CHECK(glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, attachment_id));
                GL_CHECK(glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_config.ms_samples, gl_format, m_width, m_height, GL_FALSE));
            } break;
        }

        GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + slot, gl_target, attachment_id, 0));
        m_color_attachments[slot] = attachment_id;
    }

    /* Generate depth attachment */
    if(m_config.depth_attachment_set) {
        FboDepthConfig depth = m_config.depth_attachment;
        
        int32_t gl_target = 0;
        int32_t gl_format = gl_internal_format_from_texture_data_format(depth.format);

        uint32_t attachment_id = 0;
        switch(depth.target) {
            default: { ASSERT(false); fprintf(stderr, "[Error] Framebuffer: Invalid depth attachment target!\n"); } return;

            case FboAttachmentTarget::TEXTURE: {
                gl_target = GL_TEXTURE_2D;

                GL_CHECK(glCreateTextures(GL_TEXTURE_2D, 1, &attachment_id));
                GL_CHECK(glBindTexture(GL_TEXTURE_2D, attachment_id));
                GL_CHECK(glTexStorage2D(GL_TEXTURE_2D, 1, gl_format, m_width, m_height));

                GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
                GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
                GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
                GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
                GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
            } break;

            case FboAttachmentTarget::TEXTURE_MULTISAMPLE: {
                gl_target = GL_TEXTURE_2D_MULTISAMPLE;

                GL_CHECK(glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &attachment_id));
                GL_CHECK(glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, attachment_id));
                GL_CHECK(glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_config.ms_samples, gl_format, m_width, m_height, GL_FALSE));
            } break;
        }

        GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, gl_target, attachment_id, 0));
        m_depth_attachment = attachment_id;
    }

    this->set_draw_buffers();

    m_attachments_generated = true;
}

void Framebuffer::delete_attachments(void) {
    for(uint32_t slot = 0; slot < m_config.color_attachment_count; ++slot) {
        GL_CHECK(glDeleteTextures(1, &m_color_attachments[slot]));
    }

    if(m_config.depth_attachment_set) {
        glDeleteTextures(1, &m_depth_attachment);
    }

    memset(m_color_attachments, 0, FBO_COLOR_MAX);
    m_depth_attachment = 0;
    m_attachments_generated = false;
}
