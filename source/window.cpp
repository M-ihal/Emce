#include "window.h"
#include "input.h"

#include <stdio.h>
#include <SDL.h>

namespace {
	static bool s_has_any_window_been_created = false;
}

Window::Window(void) {
	m_sdl_window  = NULL;
	m_gl_context  = NULL;
	m_width       = 0;
	m_height      = 0;
	m_should_quit = false;
}

Window::~Window(void) {
	if(!m_sdl_window && !m_gl_context) {
		return;
	}

	if(m_gl_context) {
		SDL_GL_DeleteContext(m_gl_context);
		m_gl_context = NULL;
	}

	if(m_sdl_window) {
		SDL_DestroyWindow(m_sdl_window);
		m_sdl_window = NULL;
	}

	fprintf(stdout, "[info] Window: Window destroyed.\n");
}

bool Window::initialize(int width, int height, const char *title) {
	ASSERT(::s_has_any_window_been_created == false, "Only one window allowed.\n");
	ASSERT(m_sdl_window == NULL);
	ASSERT(m_gl_context == NULL);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	const int x_pos = SDL_WINDOWPOS_UNDEFINED; 
	const int y_pos = SDL_WINDOWPOS_UNDEFINED; 

	unsigned int window_flags = SDL_WINDOW_OPENGL;

	m_sdl_window = SDL_CreateWindow(title, x_pos, y_pos, width, height, window_flags);
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
	::s_has_any_window_been_created = true;

	/* Initialize window size */
	SDL_GetWindowSize(m_sdl_window, &m_width, &m_height);

	return true;
}

void Window::process_events(Input &input) {
	input.prepare_to_catch_input();

	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_QUIT: {
				m_should_quit = true;
			} continue;

			case SDL_WINDOWEVENT: {
                const SDL_WindowEvent &window_event = event.window;
                switch(window_event.event) {
                    // case SDL_WINDOWEVENT_RESIZED:
                    case SDL_WINDOWEVENT_SIZE_CHANGED: {
                        m_width  = window_event.data1;
                        m_height = window_event.data2;
                    } break;
                }
            } continue;
		}

		input.catch_input(event);
	}
}

void Window::swap_buffers(void) {
	SDL_GL_SwapWindow(m_sdl_window);
}

void Window::should_quit(void) {
	m_should_quit = true;
}

bool Window::is_running(void) const {
	return !m_should_quit;
}

int32_t Window::width(void) const {
	return m_width;
}

int32_t Window::height(void) const {
	return m_height;
}