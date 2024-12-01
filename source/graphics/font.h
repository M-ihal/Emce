#pragma once

#include "common.h"
#include "math_types.h"
#include "texture.h"
#include "utils.h"

#include <meh_hash.h>

inline uint64_t font_glyph_table_hash_func(const int32_t &key) {
    uint64_t hash = (uint64_t)*(uint32_t *)&key;
    hash *= 31;
    return hash;
}

inline bool font_glyph_table_comp_func(const int32_t &key_a, const int32_t &key_b) {
    return key_a == key_b;
}

class Font {
public:
    CLASS_COPY_DISABLE(Font);

    Font(void) = default;

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

    typedef meh::Table<int32_t, Glyph, font_glyph_table_hash_func, font_glyph_table_comp_func, 100> FontGlyphTable;
    
    /* Loads font from .ttf file */
    bool load_from_file(const char *filepath, int32_t height_in_pixels);
    
    /* Deletes allocated structs and the atlas texture */
    void delete_font(void);

    /* Fill glyph structure with data */
    bool get_glyph(int32_t codepoint, Glyph &glyph);

    /* Get advance value */
    int32_t get_kerning_advance(int32_t codepoint_left, int32_t codepoint_right);

    /* Access font atlas texture */
    Texture &get_atlas(void);
 
    /* Get scale value */
    float get_scale_for_pixel_height(void);

    int32_t get_height(void);
    int32_t get_line_gap(void);
    int32_t get_descent(void);
    int32_t get_ascent(void);

    float calc_text_width(const char *text);

private:
    float   m_scale_for_pixel_height;
    int32_t m_height;
    int32_t m_ascent;
    int32_t m_descent;
    int32_t m_line_gap;
    
    FileContents m_font_file; // @allocated
    void   *m_font_stb_info;  // @allocated
    Texture m_atlas;
    
    /* @TODO: probably should just use flat array.. */
    FontGlyphTable m_glyph_table;
};

