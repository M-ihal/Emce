#pragma once

#include "common.h"
#include "texture.h"

#include <string>

class Cubemap {
public:
    CLASS_COPY_DISABLE(Cubemap);

    Cubemap(void);

    void delete_cubemap(void);

    /* Function takes array of 6 std::strings to images of cubemap in following order:
        - right
        - left
        - top
        - bottom
        - front
        - back
    */
    bool load_from_file(const std::string (&filepaths)[6], bool flip_on_load = false);

    void bind_cubemap(void);

    void set_filter_min(TextureFilter param);
    void set_filter_mag(TextureFilter param);

    void set_wrap_s(TextureWrap param);
    void set_wrap_t(TextureWrap param);
    void set_wrap_r(TextureWrap param);

private:
    uint32_t m_cubemap_id;
    TextureFilter m_filter_min;
    TextureFilter m_filter_mag;
    TextureWrap m_wrap_t;
    TextureWrap m_wrap_s;
    TextureWrap m_wrap_r;
};
