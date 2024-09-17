#pragma once

#include "common.h"
#include "math_types.h"
#include "texture.h"
#include "utils.h"

// meh
#include <unordered_map>

class Font {
public:
    struct Glyph {
        bool    has_glyph;
        int32_t width;
        int32_t height;
        int32_t offset_x;
        int32_t offset_y;
        int32_t advance;
        int32_t left_side_bearing;
        vec2    tex_coords[4];
    };

public:
    CLASS_COPY_DISABLE(Font);
    
    Font(void);

    bool load_from_file(const char *filepath, int32_t height_in_pixels);
    
    /* Deletes allocated structs and the atlas texture */
    void delete_font(void);

    int32_t get_kerning_advance(int32_t codepoint_left, int32_t codepoint_right) const;

    // @temp
    bool get_glyph(int32_t codepoint, Glyph &glyph) const {
        auto slot = m_glyphs.find(codepoint);
        if(slot == m_glyphs.end()) {
            return false;
        }

        glyph = slot->second;
        return true;
    }
    
    Texture &get_atlas(void) {
        return m_atlas;
    }
    
    const Texture &get_atlas(void) const {
        return m_atlas;
    }

    float get_scale_for_pixel_height(void) const {
        return m_scale_for_pixel_height;
    }

    int32_t get_height(void) const {
        return m_height;
    }

    int32_t get_line_gap(void) const {
        return m_line_gap;
    }

    int32_t get_descent(void) const {
        return m_descent;
    }

    int32_t get_ascent(void) const {
        return m_ascent;
    }


private:
    float   m_scale_for_pixel_height;
    int32_t m_height;
    int32_t m_ascent;
    int32_t m_descent;
    int32_t m_line_gap;
    
    FileContents m_font_file; // @allocated
    void   *m_font_stb_info;  // @allocated
    Texture m_atlas;
    std::unordered_map<int32_t, Glyph> m_glyphs; // @todo How to delete this
};
