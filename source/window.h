#pragma once

#include "common.h"

class         Input;
struct        SDL_Window;
typedef void *SDL_GLContext;

class Window {
public:
    CLASS_COPY_DISABLE(Window);

    explicit Window(void);
    ~Window(void);

    bool initialize(int width, int height, const char *title);
    void process_events(Input &input);
    void swap_buffer(void);

    void set_should_close(void);
    bool get_should_close(void) const;

    float   calc_aspect(void) const;
    int32_t get_width(void) const;
    int32_t get_height(void) const;

    /* Return OS handle i.e. on windows HWND */
    void *get_os_native_handle(void);

private:
    SDL_Window    *m_sdl_window;
    SDL_GLContext  m_gl_context;
    int32_t        m_width;
    int32_t        m_height;
    bool           m_should_close;
};
