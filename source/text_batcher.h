#pragma once

#include "common.h"
#include "math_types.h"
#include "font.h"

#define TEXT_BATCHER_QUAD_MAX 4096

namespace TextBatcher {
    void initialize(void);
    void destroy(void);
    void begin(void);
    void render(int32_t width, int32_t height, const Font &font, vec2i shadow_offset = vec2i::zero());
    void push_text(vec2 position, const Font &font, const char *string);
    void push_text_formatted(vec2 position, const Font &font, const char *string, ...);
}
