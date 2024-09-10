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

	void swap_buffers(void);

	void should_quit(void);

	bool is_running(void) const;

	int32_t width(void) const;

	int32_t height(void) const;

private:
	SDL_Window    *m_sdl_window;
	SDL_GLContext  m_gl_context;
	int32_t        m_width;
	int32_t        m_height;
	bool           m_should_quit;
};
