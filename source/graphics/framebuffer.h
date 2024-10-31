#pragma once

#include "common.h"
#include "texture.h"

#define FBO_COLOR_MAX 8

enum class FboAttachmentTarget {
    NONE,
    TEXTURE,
    TEXTURE_MULTISAMPLE,
};

struct FboColorConfig {
    FboAttachmentTarget target;
    TextureDataFormat   format;
};

struct FboDepthConfig {
    FboAttachmentTarget target;
    TextureDataFormat   format;
};

struct FboConfig {
    uint32_t       ms_samples = 1; /* For ms attachments */
    uint32_t       color_attachment_count = 0;
    FboColorConfig color_attachments[FBO_COLOR_MAX];
    bool           depth_attachment_set = false;
    FboDepthConfig depth_attachment;
};

void bind_no_fbo(void);

class Framebuffer {
public:
    CLASS_COPY_DISABLE(Framebuffer);

    Framebuffer(void) = default;

    void bind_fbo(void);

    bool create_fbo(int32_t width, int32_t height, FboConfig &config);

    void delete_fbo(void);

    void resize_fbo(int32_t width, int32_t height);

    void clear_all(const vec4 &color, float depth_value = 1.0f, int32_t stencil_value = 0);

    void clear_color(uint32_t slot, const vec4 &color);

    void clear_depth(float depth_value = 1.0f, int32_t stencil_value = 0);

    uint32_t get_fbo_id(void);

    uint32_t get_color_attachment_id(uint32_t slot);

    uint32_t get_depth_attachment_id(void);

    bool is_complete(void); /* Is fbo valid && are attachments generated */

    int32_t get_width(void);

    int32_t get_height(void);

    vec2i get_size(void);

private:
    void gen_attachments(void);
    
    void delete_attachments(void);

    uint32_t m_fbo_id = 0;
    int32_t  m_width;
    int32_t  m_height;

    FboConfig m_config;
    bool m_attachments_generated = false;
    uint32_t m_color_attachments[FBO_COLOR_MAX];
    uint32_t m_depth_attachment = 0;
};

inline void fbo_config_push_color(FboConfig &config, FboColorConfig color) {
    ASSERT(config.color_attachment_count + 1 < FBO_COLOR_MAX, "FboConfig: Color attachment limit exceeded.");

    uint32_t slot = config.color_attachment_count++;
    config.color_attachments[slot] = color;
}

inline void fbo_config_set_depth(FboConfig &config, FboDepthConfig depth) {
    config.depth_attachment_set = true;
    config.depth_attachment = depth;
}
