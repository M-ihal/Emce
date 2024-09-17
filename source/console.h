#pragma once

#include "common.h"
#include "input.h"

namespace Console {
    void initialize(void);
    void destroy(void);
    void update(const Input &input);
    void render(int32_t window_w, int32_t window_h);
    void set_open_state(bool open);
    bool is_open(void);
}
