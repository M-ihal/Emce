#pragma once

#include "common.h"
#include "texture.h"

enum ClearFlags : uint32_t {
    CLEAR_COLOR   = 0x1,
    CLEAR_DEPTH   = 0x2,
    CLEAR_STENCIL = 0x4
};

class Framebuffer {
public:
    CLASS_COPY_DISABLE(Framebuffer);

    Framebuffer(void);
    
    /* Unbinds any framebuffer */
    static void bind_no_fbo(void);

    /* Binds this framebuffer */
    void bind_fbo(void);

    /* Create framebuffer */
    bool create_fbo(int32_t width, int32_t height);

    /* Delte framebuffer */
    void delete_fbo(void);

    /* Resize the framebuffer */
    void resize_fbo(int32_t new_width, int32_t new_height);
    
    /* Sets viewport to cover whole framebuffer */
    void set_viewport(void);

    /* Clear the framebuffer */
    void clear_fbo(const vec4 &color, uint32_t flags);
    
    /* Render whole fbo on bound framebuffer */
    void blit_whole(int32_t width, int32_t height);

    const Texture &get_color_texture(void) const;
    const Texture &get_depth_texture(void) const;

    int32_t get_width(void)  const;
    int32_t get_height(void) const;
    vec2i   get_size(void)   const;

    uint32_t get_fbo_id(void) {
        return m_fbo_id;
    }

private:
    uint32_t m_fbo_id;
    Texture  m_color;
    Texture  m_depth;
    int32_t  m_width;
    int32_t  m_height;
};
