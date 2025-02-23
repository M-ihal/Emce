#include "window.h"
#include "input.h"

#include <stdio.h>
#include <stdexcept>

#include <SDL3/SDL.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#endif

std::unique_ptr<Window> Window::_window = NULL;

Window::Window(void) {
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4); // 3
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5); // 3

    unsigned int window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;

    m_sdl_window = SDL_CreateWindow(INIT_WINDOW_TITLE, INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT, window_flags);
    if(!m_sdl_window) {
        fprintf(stderr, "[error] Window: Failed to create SDL window.\n");
        throw std::exception();
    }

    m_gl_context = SDL_GL_CreateContext(m_sdl_window);
    if(!m_gl_context) {
        fprintf(stderr, "[error] Window: Failed to create GL Context.\n");
        throw std::exception();
    }

    fprintf(stdout, "[info] Window: Window created.\n");

    /* Initialize window size */
    SDL_GetWindowSize(m_sdl_window, &m_width, &m_height);

    /* Set vsync */
    SDL_GL_SetSwapInterval(INIT_ENABLE_VSYNC ? 1 : 0);

    /* Do not query text input by default */
    this->set_text_input_active(false);

    /* Do not capture mouse by default */
    this->set_rel_mouse_active(false);

    m_should_close = false;
    m_size_changed_this_frame = true;
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

Window &Window::get(void) {
    if(Window::_window == NULL) {
        try {
            _window = std::unique_ptr<Window>(new Window());
        } catch(const std::exception &exc) {
            throw exc;
        }
    }
    return *_window;
}

void Window::process_events(Input &input) {
    input.prepare_to_catch_input();
    
    m_size_changed_this_frame = false;

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
                m_size_changed_this_frame = true;
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

double Window::get_aspect(void) const {
    return (double)m_width / (double)m_height;
}

bool Window::size_changed_this_frame(void) const {
    return m_size_changed_this_frame;
}

void Window::set_text_input_active(bool enable) {
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

void Window::set_rel_mouse_active(bool enable) {
    if(enable == this->is_rel_mouse_active()) {
        return;
    }

    SDL_SetWindowRelativeMouseMode(m_sdl_window, enable);
}

bool Window::is_rel_mouse_active(void) {
    return SDL_GetWindowRelativeMouseMode(m_sdl_window);
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
