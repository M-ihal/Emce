#pragma once

#include "common.h"
#include "math_types.h"

class Texture {
public:
    CLASS_COPY_DISABLE(Texture);

    /* Create empty texture */
    Texture(int32_t width, int32_t height, int32_t bpp, int32_t internal_format);
    Texture(uint8_t *pixels, int32_t width, int32_t height, int32_t bpp, int32_t internal_format, int32_t data_format, int32_t data_type);
    Texture(const char *filepath);
    ~Texture(void);

    void bind_texture(void);
    void bind_texture_unit(int32_t unit);

    vec2i size(void);

    void set_pixels(uint8_t *pixels, int32_t width, int32_t height, int32_t offset_x, int32_t offset_y, int32_t data_format, int32_t data_type);

    void set_filter_min(int32_t filter);
    void set_filter_mag(int32_t filter);
    void set_wrap_s(int32_t wrap);
    void set_wrap_t(int32_t wrap);

private:
    bool init_empty_texture(int32_t width, int32_t height, int32_t bpp, int32_t internal_format);

    uint32_t m_texture_id;
    int32_t  m_width;
    int32_t  m_height;
    int32_t  m_bpp;

    /* Replace with an enum class to not need to include glew.h */
    int32_t m_internal_format;
    int32_t m_filter_min;
    int32_t m_filter_mag;
    int32_t m_wrap_s;
    int32_t m_wrap_t;
};
