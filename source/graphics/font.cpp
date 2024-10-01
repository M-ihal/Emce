#include "font.h"
#include "utils.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

Font::Font(void) {
    m_scale_for_pixel_height = 0.0f;
    m_height = 0;
    m_ascent = 0;
    m_descent = 0;
    m_line_gap = 0;
    m_font_stb_info = NULL;
}

bool Font::load_from_file(const char *filepath, int32_t height_in_pixels) {
    fprintf(stdout, "[info] Font: Loading font from ttf file, path: %s\n", filepath);

    FileContents font_file;
    if(!read_entire_file(filepath, font_file, true)) {
        fprintf(stderr, "[error] Font: Failed to read font file.\n");
        return false;
    }

    stbtt_fontinfo *font_info = (stbtt_fontinfo *)malloc(sizeof(stbtt_fontinfo));
    if(!stbtt_InitFont(font_info, font_file.data, 0)) {
        free_loaded_file(font_file);
        return false;
    }

    /* Keep the stb struct and loaded file in memory */
    m_font_stb_info = (void *)font_info;
    m_font_file = font_file;

    /* Get basic font variables @todo Check stuff about the font sizing... */
    m_height = height_in_pixels;
    m_scale_for_pixel_height = stbtt_ScaleForMappingEmToPixels(font_info, height_in_pixels);
    // m_scale_for_pixel_height = stbtt_ScaleForPixelHeight(font_info, height_in_pixels);
    stbtt_GetFontVMetrics(font_info, &m_ascent, &m_descent, &m_line_gap);

    /* @todo Hardcoded, Allocate font atlas bitmap */
    const int32_t atlas_w = 1024;
    const int32_t atlas_h = 1024;
    uint8_t *atlas_bitmap = (uint8_t *)malloc(atlas_w * atlas_h);
    memset(atlas_bitmap, 0, atlas_w * atlas_h);
    ASSERT(atlas_bitmap, "load_font_from_file: Failed to allocate memory.\n");

    /* Init stb rect pack stuff */
    const int32_t num_nodes = 1024;
    stbrp_node *rp_nodes = (stbrp_node *)malloc(num_nodes * sizeof(stbrp_node));
    ASSERT(rp_nodes, "load_font_from_file: Failed to allocate memory.\n");
    stbrp_context rect_pack_context;
    stbrp_init_target(&rect_pack_context, atlas_w, atlas_h, rp_nodes, num_nodes);

    const float fatlas_w = float(atlas_w);
    const float fatlas_h = float(atlas_h);

    /* @todo Try to load all ASCII characters for now */
    for(int32_t codepoint = 0; codepoint  < 255; ++codepoint) {
        Font::Glyph glyph = { };
        stbtt_GetCodepointHMetrics(font_info, codepoint, &glyph.advance, &glyph.left_side_bearing);

        uint8_t *glyph_bitmap = stbtt_GetCodepointBitmap(font_info, 0, m_scale_for_pixel_height, codepoint, &glyph.width, &glyph.height, &glyph.offset_y, &glyph.offset_y);
        if(!glyph_bitmap) {
            /* No bitmap for this character -> Do not add @todo */
            m_glyphs.insert({ codepoint, glyph });
            continue;
        }

        /* If got here, the glyph has bitmap data */
        glyph.has_glyph = true;

        stbrp_rect glyph_rect;
        glyph_rect.w = glyph.width + 2;  // @hack +2
        glyph_rect.h = glyph.height + 2; // @hack +2
        stbrp_pack_rects(&rect_pack_context, &glyph_rect, 1);
        if(!glyph_rect.was_packed) {
            /* Failed to pack glyph rect for some reason @todo */
            free(glyph_bitmap);
            continue;
        }

        // @todo @hack Quick solution to edge artefacts when rendering, leaving one pixel border around each glyph
        glyph_rect.w -= 2;
        glyph_rect.h -= 2;
        glyph_rect.x += 1;
        glyph_rect.y += 1;

        /* Copy glyph bitmap to the assigned location in atlas bitmap, also flip it @todo */
        for(int32_t y = 0; y < glyph_rect.h; ++y) {
            for(int32_t x = 0; x < glyph_rect.w; ++x) {
                const int32_t dest_pixel = (glyph_rect.y + y) * atlas_w + (glyph_rect.x + x);
                const int32_t src_pixel  = (glyph_rect.h - y - 1) * glyph_rect.w + x;
                atlas_bitmap[dest_pixel] = glyph_bitmap[src_pixel];
            }
        }

        /* Calculate texture coords @todo */
        glyph.tex_coords[0] = { glyph_rect.x / fatlas_w,                   glyph_rect.y / fatlas_h };
        glyph.tex_coords[1] = { (glyph_rect.x + glyph_rect.w) / fatlas_w,  glyph_rect.y / fatlas_h };
        glyph.tex_coords[2] = { (glyph_rect.x + glyph_rect.w) / fatlas_w, (glyph_rect.y + glyph_rect.h) / fatlas_h };
        glyph.tex_coords[3] = { glyph_rect.x / fatlas_w,                  (glyph_rect.y + glyph_rect.h) / fatlas_h };

        free(glyph_bitmap);
        m_glyphs.insert({ codepoint, glyph });
    }

    TextureLoadSpec spec = {
        .internal_format = TextureDataFormat::RED,
        .data_format     = TextureDataFormat::RED,
        .data_type       = TextureDataType::UNSIGNED_BYTE
    };

    const bool success = m_atlas.load_from_memory(atlas_bitmap, atlas_w, atlas_h, spec);
    ASSERT(success);
    
    free(rp_nodes);
    free(atlas_bitmap);

    return false;
}

void Font::delete_font(void) {
    m_atlas.delete_texture_if_exists();
    free_loaded_file(m_font_file);
    free(m_font_stb_info);
    m_glyphs.clear();
}

int32_t Font::get_kerning_advance(int32_t codepoint_left, int32_t codepoint_right) const {
    return stbtt_GetGlyphKernAdvance((stbtt_fontinfo *)m_font_stb_info, codepoint_left, codepoint_right);
}

// @TODO: Copied!!!!
float Font::calc_text_width(const char *text) const {
    const char  *string = text;
    const size_t length = strlen(string);

    float text_width = 0.0f;
    for(int32_t index = 0; index < length; ++index) {
        const char _char = string[index];
        Font::Glyph glyph;

        /* @todo Special characters not handled... */
        switch(_char) {
            case '\n':
            case '\r':
            case '\t': {
                INVALID_CODE_PATH;
                continue;
            };
        }

        if(!this->get_glyph(_char, glyph)) {
            /* No glyph entry in the hash */
            continue;
        }

        const int32_t adv = glyph.advance + this->get_kerning_advance(_char, string[index + 1]);
        text_width += float(adv) * this->get_scale_for_pixel_height();
    }

    return text_width;
}
