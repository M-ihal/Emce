#include "window.h"
#include "input.h"

#include <stdio.h>

#include <SDL3/SDL.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#endif

namespace {
    static bool g_has_any_window_been_created = false;
}

Window::Window(void) {
    m_sdl_window   = NULL;
    m_gl_context   = NULL;
    m_width        = 0;
    m_height       = 0;
    m_should_close = false;
}

Window::~Window(void) {
    if(!m_sdl_window && !m_gl_context) {
        return;
    }

    if(m_gl_context) {
        SDL_GL_DestroyContext(m_gl_context);
        m_gl_context = NULL;
    }

    if(m_sdl_window) {
        SDL_DestroyWindow(m_sdl_window);
        m_sdl_window = NULL;
    }

    fprintf(stdout, "[info] Window: Window destroyed.\n");
}

bool Window::initialize(int width, int height, const char *title) {
    ASSERT(::g_has_any_window_been_created == false, "Only one window allowed.\n");
    ASSERT(!m_sdl_window);
    ASSERT(!m_gl_context);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    unsigned int window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;

    m_sdl_window = SDL_CreateWindow(title, width, height, window_flags);
    if(!m_sdl_window) {
        fprintf(stderr, "[error] Window: Failed to create SDL window.\n");
        return false;
    }

    m_gl_context = SDL_GL_CreateContext(m_sdl_window);
    if(!m_gl_context) {
        fprintf(stderr, "[error] Window: Failed to create GL Context.\n");
        return false;
    }

    fprintf(stdout, "[info] Window: Window created.\n");
    ::g_has_any_window_been_created = true;

    /* Initialize window size */
    SDL_GetWindowSize(m_sdl_window, &m_width, &m_height);

    /* Set vsync */
    SDL_GL_SetSwapInterval(1);

    return true;
}

void Window::process_events(Input &input) {
    input.prepare_to_catch_input();

    SDL_Event event;
    while(SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_EVENT_QUIT: {
                m_should_close = true;
            } continue;

            case SDL_EVENT_WINDOW_RESIZED: {
                const SDL_WindowEvent &window_event = event.window;
                m_width  = window_event.data1;
                m_height = window_event.data2;
            } continue;
        }

        input.catch_input(event);
    }
}

void Window::swap_buffer(void) {
    SDL_GL_SwapWindow(m_sdl_window);
}

void Window::set_should_close(void) {
    m_should_close = true;
}

bool Window::get_should_close(void) const {
    return !m_should_close;
}

int32_t Window::get_width(void) const {
    return m_width;
}

int32_t Window::get_height(void) const {
    return m_height;
}

float Window::calc_aspect(void) const {
    return float(m_width) / float(m_height);
}

void Window::set_text_input_active(bool enable) {
    /* Check if trying to set to the same state */
    if(enable == this->is_text_input_active()) {
        return;
    }

    if(enable) {
        SDL_StartTextInput(m_sdl_window);
    } else {
        SDL_StopTextInput(m_sdl_window);
    }
}

bool Window::is_text_input_active(void) const {
    return SDL_TextInputActive(m_sdl_window);
}

void *Window::get_os_native_handle(void) {
#if defined(_WIN32)
    SDL_PropertiesID props = SDL_GetWindowProperties(m_sdl_window);
    ASSERT(props != 0);
    void *hwnd = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    return hwnd;
#else
    INVALID_CODE_PATH;
#endif
}

