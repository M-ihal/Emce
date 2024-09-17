#pragma once

#include "common.h"
#include "math_types.h"
#include "font.h"

#define TEXT_BATCHER_QUAD_MAX 4096

struct TextQuadVertex {
    vec2 position;
    vec2 tex_coord;
};

struct TextBatcher {
public:
    CLASS_COPY_DISABLE(TextBatcher);

    /* Initializes and destroys static global variables */
    static void initialize(void);
    static void destroy(void);
    static void hotload_shader(void);

    /* Allocates and deletes memory */
    TextBatcher(void);
    ~TextBatcher(void);

    void begin(void);
    void render(int32_t window_w, int32_t window_h, const Font &font, vec2i shadow_offset = vec2i::zero());
    void push_text(vec2 position, const Font &font, const char *string);
    void push_text_formatted(vec2 position, const Font &font, const char *string, ...);
    void push_text_va_args(vec2 position, const Font &font, const char *string, va_list args);

private:
    TextQuadVertex *m_vertices;
    uint32_t m_chars_pushed;
};
