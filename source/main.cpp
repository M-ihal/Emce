#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <glew.h>

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "window.h"
#include "input.h"
#include "shader.h"
#include "vertex_array.h"
#include "math_types.h"

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

        layout (location = 0) in vec3 a_position;
        layout (location = 1) in vec3 a_color;

        uniform mat4 u_proj;

        out vec3 v_color;

        void main() {
            v_color = a_color;
            gl_Position = u_proj * vec4(a_position, 1.0);
        }
    )";

    const char fs[] = R"(
        #version 330 core

        in vec3 v_color;
        out vec4 f_color;

        void main() {
            f_color = vec4(v_color, 1);
        }
    )";

    Shader shader;
    if(!shader.load_from_memory(vs, strlen(vs), fs, strlen(fs))) {
        return -1;
    }

    ASSERT(shader.is_valid_program());

    shader.use_program();

    BufferLayout layout;
    layout.push_attribute("a_position", 3, GL_FLOAT, 4);
    layout.push_attribute("a_color", 3, GL_FLOAT, 4);

    float verts[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.5f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    };

    uint32_t inds[] = {
        0, 1, 2
    };

    VertexArray vao;
    vao.add_vertex_buffer(verts, ARRAY_COUNT(verts) * sizeof(float), ArrayBufferUsage::STATIC, layout);
    vao.add_index_buffer(inds, ARRAY_COUNT(inds), ArrayBufferUsage::STATIC);
    vao.apply_vertex_attributes();

    mat4 projection = mat4::orthographic(0.0f, 0.0f, 1.0f, 1.0f, -1.0f, 1.0f);

    while(window.is_running()) {
        window.process_events(input);

        if(input.key_pressed(Key::ESCAPE)) {
            window.should_quit();
        }

        glViewport(0, 0, window.width(), window.height());

        GL_CHECK(glClearColor(0.2f, 0.2f, 0.4f, 1.0));
        GL_CHECK(glClear(GL_COLOR_BUFFER_BIT));

        PRINT_INT(window.width());
        PRINT_INT(window.height());

        shader.use_program();
        shader.upload_mat4("u_proj", projection.e);

        vao.bind_vao();

        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);

        window.swap_buffers();
    }

    return 0;
}
