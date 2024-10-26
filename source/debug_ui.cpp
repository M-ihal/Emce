#include "debug_ui.h"
#include "text_batcher.h"
#include "font.h"
#include "framebuffer.h"

#include <cstdarg>

namespace {
    bool        g_initialized = false;
    Font        g_font;
    TextBatcher g_batcher; // @todo Make sure it calls destructor

    const float g_spacing = 8.0f;
    const vec2  g_padding = { 4.0f, 4.0f };

    bool g_show = false;

    /* Frame data */
    float g_left_x;
    float g_left_y;
    float g_right_x;
    float g_right_y;
    float g_window_w;
    float g_window_h;
};

void DebugUI::initialize(void) {
    ASSERT(!g_initialized, "DebugUI: Already initialized.\n");
    g_initialized = true;

    const char *font_filepath = "C://dev//emce//data//CascadiaMono.ttf";

    g_font.load_from_file(font_filepath, 16);
    g_font.get_atlas().set_filter_min(TextureFilter::NEAREST);
    g_font.get_atlas().set_filter_mag(TextureFilter::NEAREST);
}

void DebugUI::destroy(void) {
    g_font.delete_font();
    g_initialized = false;
}

void DebugUI::toggle(void) {
    BOOL_TOGGLE(g_show);
}

void DebugUI::begin_frame(int32_t window_w, int32_t window_h) {
    g_batcher.begin();

    g_window_w = float(window_w);
    g_window_h = float(window_h);

    const float baseline_y = g_window_h - g_padding.y - g_font.get_ascent() * g_font.get_scale_for_pixel_height();

    g_left_x = g_padding.x;
    g_left_y = baseline_y;
    g_right_x = window_w - g_padding.x - 400.0f; // @temp
    g_right_y = baseline_y;
}


void DebugUI::push_text_left(const char *format, ...) {
#define LEFT_SKIP_LINE() g_left_y -= g_font.get_height() + g_spacing
    /* Skip line */
    if(!format) {
        LEFT_SKIP_LINE();
        return;
    }

    va_list args;
    va_start(args, format);
    g_batcher.push_text_va_args(vec2{ g_left_x, g_left_y }, g_font, format, args);
    va_end(args);
    LEFT_SKIP_LINE();
}

void DebugUI::push_text_right(const char *format, ...) {
#define RIGHT_SKIP_LINE() g_right_y -= g_font.get_height() + g_spacing
    if(!format) {
        RIGHT_SKIP_LINE();
        return;
    }

    // @todo Align right stuff...

    va_list args;
    va_start(args, format);
    g_batcher.push_text_va_args(vec2{ g_right_x, g_right_y }, g_font, format, args);
    va_end(args);
    RIGHT_SKIP_LINE();
}

void DebugUI::render(void) {
    if(!g_show) {
        return;
    }

    const vec3  color = { 0.9f, 0.9f, 0.875f };
    const vec2i shadow_offset = { 2, -2 };
    g_batcher.render(g_window_w, g_window_h, g_font, color, shadow_offset);
}
