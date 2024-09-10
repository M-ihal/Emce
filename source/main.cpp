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
#include "camera.h"

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
        uniform mat4 u_view;
        uniform mat4 u_model;

        out vec3 v_color;

        void main() {
            v_color = a_color;
            gl_Position = u_proj * u_view * u_model * vec4(a_position, 1.0);
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

    Camera camera;
    camera.set_position({ 0.0f, 0.0f, -5.0f });
    camera.set_rotation({ DEG_TO_RAD(90.0f), 0.0f });

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

    while(window.is_running()) {
        window.process_events(input);

        if(input.key_pressed(Key::ESCAPE)) {
            window.should_quit();
        }


        vec3 pos = camera.get_position();

        if(input.key_is_down(Key::W)) {
            pos.z += 0.05f;
        }

        if(input.key_is_down(Key::S)) {
            pos.z -= 0.05f;
        }

        if(input.key_is_down(Key::A)) {
            pos.x -= 0.05f;
        }

        if(input.key_is_down(Key::D)) {
            pos.x += 0.05f;
        }

        vec2 rot = camera.get_rotation();

        if(input.key_is_down(Key::LEFT)) {
            rot.x -= 0.05f;
        }

        if(input.key_is_down(Key::RIGHT)) {
            rot.x += 0.05f;
        }

        camera.set_rotation(rot);
        camera.set_position(pos);

        mat4 proj_m = camera.calc_proj((float)window.width() / (float)window.height());
        mat4 view_m = camera.calc_view();

    //        PRINT_FLOAT(camera.rotation_x());

    //PRINT_FLOAT(camera.m_position.x);
    //PRINT_FLOAT(camera.m_position.z);
        glViewport(0, 0, window.width(), window.height());

        GL_CHECK(glClearColor(0.2f, 0.2f, 0.4f, 1.0));
        GL_CHECK(glClear(GL_COLOR_BUFFER_BIT));

        shader.use_program();
        shader.upload_mat4("u_proj", proj_m.e);
        shader.upload_mat4("u_view", view_m.e);
        shader.upload_mat4("u_model", mat4::identity().e);

        vao.bind_vao();

        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);

        window.swap_buffers();
    }

    return 0;
}
