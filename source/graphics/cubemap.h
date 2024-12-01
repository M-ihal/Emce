#pragma once

#include "common.h"
#include "texture.h"

#include <string>

class Cubemap {
public:
    CLASS_COPY_DISABLE(Cubemap);

    Cubemap(void) = default;

    /* Bind cubemap */
    void bind_cubemap(void);

    /* Bind cubemap to slot */
    void bind_cubemap_unit(uint32_t slot);

    /* Delete cubemap ID */
    void delete_cubemap(void);

    /* Function takes array of 6 std::strings to RGB images of cubemap in following order:
        - right
        - left
        - top
        - bottom
        - front
        - back
    */
    bool load_from_file(const std::string (&filepaths)[6], bool flip_on_load = false);

    /* Filter settings */
    void set_filter_min(TextureFilter param);
    void set_filter_mag(TextureFilter param);

    /* Wrap settings */
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
