#pragma once

#include "common.h"

namespace DebugUI {
    void initialize(void);
    void destroy(void);
    void toggle(void);
    void begin_frame(int32_t window_w, int32_t window_h);
    void push_text_left(const char *format, ...);
    void push_text_right(const char *format, ...);
    void render(void);
};
