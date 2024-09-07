#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <glew.h>

#include "common.h"
#include "window.h"
#include "input.h"
#include "shader.h"
#include "vertex_array.h"

int SDL_main(int argc, char *argv[]) {
	const bool sdl_success = SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO) == 0;
	if(!sdl_success) {
		fprintf(stderr, "[error] SDL: Failed to initialize SDL, Error: %s.\n", SDL_GetError());
		return -1;
	}

	Window window;
	Input  input;

	if(!window.initialize(900, 600, "emce")) {
		fprintf(stderr, "[error] Window: Failed to create valid opengl window.\n");
		return -1;
	}

	const bool glew_success = glewInit() == GLEW_OK;
	if(!glew_success) {
		fprintf(stderr, "[error] Glew: Failed to initialize.\n");
		return -1;
	}

	const char vs[] = R"(
		#version 330 core

		uniform float u_value;

		void main() {
			gl_Position = vec4(0, 0, u_value, 1);
		}
	)";

	const char fs[] = R"(
		#version 330 core

		out vec4 f_color;

		void main() {
			f_color = vec4(1, 1, 0, 1);
		}
	)";

	Shader shader;
	if(!shader.load_from_memory(vs, strlen(vs), fs, strlen(fs))) {
		return -1;
	}

	ASSERT(shader.is_valid_program());

	shader.use_program();
	shader.upload_float("u_value", 1.0f);

	Shader::use_no_program();

	VertexArray vao;

	while(window.is_running()) {
		window.process_events(input);

		if(input.key_pressed(Key::ESCAPE)) {
			window.should_quit();
		}

		glClearColor(0.2f, 0.2f, 0.2f, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);

		window.swap_buffers();
	}

	return 0;
}