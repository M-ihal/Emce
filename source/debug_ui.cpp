#include "debug_ui.h"
#include "text_batcher.h"
#include "font.h"

#include <cstdarg>

namespace {
    bool        s_initialized = false;
    Font        s_font;
    TextBatcher s_batcher; // @todo Make sure it calls destructor

    const float s_spacing = 8.0f;
    const vec2  s_padding = { 4.0f, 4.0f };

    /* Frame data */
    float s_left_x;
    float s_left_y;
    float s_right_x;
    float s_right_y;
    float s_window_w;
    float s_window_h;
};

void DebugUI::initialize(void) {
    ASSERT(!s_initialized, "DebugUI: Already initialized.\n");
    s_initialized = true;

    const char *font_filepath = "C://dev//emce//data//CascadiaMono.ttf";
    // const char *font_filepath = "C://dev//emce//data//minecraftia-regular.ttf";

    s_font.load_from_file(font_filepath, 16);
    s_font.get_atlas().set_filter_min(TextureFilter::NEAREST);
    s_font.get_atlas().set_filter_mag(TextureFilter::NEAREST);
}

void DebugUI::destroy(void) {
    s_font.delete_font();
    s_initialized = false;
}

void DebugUI::begin_frame(int32_t window_w, int32_t window_h) {
    s_batcher.begin();

    s_window_w = float(window_w);
    s_window_h = float(window_h);

    const float baseline_y = s_window_h - s_padding.y - s_font.get_ascent() * s_font.get_scale_for_pixel_height();

    s_left_x = s_padding.x;
    s_left_y = baseline_y;
    s_right_x = window_w - s_padding.x - 200.0f; // @temp 200
    s_right_y = baseline_y;
}


void DebugUI::push_text_left(const char *format, ...) {
#define LEFT_SKIP_LINE() s_left_y -= s_font.get_height() + s_spacing
    /* Skip line */
    if(!format) {
        LEFT_SKIP_LINE();
        return;
    }

    va_list args;
    va_start(args, format);
    s_batcher.push_text_va_args(vec2{ s_left_x, s_left_y }, s_font, format, args);
    va_end(args);
    LEFT_SKIP_LINE();
}

void DebugUI::push_text_right(const char *format, ...) {
#define RIGHT_SKIP_LINE() s_right_y -= s_font.get_height() + s_spacing
    if(!format) {
        RIGHT_SKIP_LINE();
        return;
    }

    // @todo Align right stuff...

    va_list args;
    va_start(args, format);
    s_batcher.push_text_va_args(vec2{ s_right_x, s_right_y }, s_font, format, args);
    va_end(args);
    RIGHT_SKIP_LINE();
}

void DebugUI::render(void) {
    s_batcher.render(s_window_w, s_window_h, s_font, vec2i{ 3, -3 });
}
