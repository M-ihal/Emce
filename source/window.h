#pragma once

#include "common.h"

#include <memory>

class   Input;
struct  SDL_Window;
struct  SDL_GLContextState;
typedef SDL_GLContextState *SDL_GLContext;

inline constexpr int32_t INIT_WINDOW_WIDTH  = 1280;
inline constexpr int32_t INIT_WINDOW_HEIGHT = 720;
inline const char       *INIT_WINDOW_TITLE  = "emce";
inline constexpr bool    INIT_ENABLE_VSYNC  = true;

class Window {
    CLASS_COPY_DISABLE(Window);

    static std::unique_ptr<Window> _window;

    Window(void);

public:
    ~Window(void);

    /* Get the window instance */
    static Window &get(void);

    /* Process window events and fill out input */
    void process_events(Input &input);

    /* Swap draw buffer */
    void swap_buffer(void);

    void set_should_close(void);
    bool get_should_close(void) const;

    /* Set if should query text input */
    void set_text_input_active(bool enable);
    bool is_text_input_active(void) const;

    /* Set if mouse should be in relative mode */
    void set_rel_mouse_active(bool enable);
    bool is_rel_mouse_active(void);

    /* Check if window's size changed last process_events call */
    bool size_changed_this_frame(void) const;

    /* Return OS handle i.e. on windows HWND */
    void *get_os_native_handle(void);

    double  get_aspect(void) const;
    int32_t get_width(void) const;
    int32_t get_height(void) const;

private:
    SDL_Window    *m_sdl_window;
    SDL_GLContext  m_gl_context;
    int32_t        m_width;
    int32_t        m_height;
    bool           m_should_close;
    bool           m_size_changed_this_frame;
};
