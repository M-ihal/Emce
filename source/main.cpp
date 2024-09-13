#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <glew.h>

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "common.h"
#include "window.h"
#include "input.h"
#include "shader.h"
#include "vertex_array.h"
#include "math_types.h"
#include "camera.h"
#include "texture.h"
#include "chunk.h"

/*
    @TODO: Game / GameManager / Game-Something class
*/

VertexArray gen_cube_vao(void) {
    struct CubeVertex {
        vec3 position;
        vec3 normal;
        vec2 tex_coord;
    };

    const CubeVertex vertices[] = {
        { -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f }, // Bottom-left
        {  0.5f,  0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f }, // top-right
        {  0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f }, // bottom-right         
        {  0.5f,  0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f }, // top-right
        { -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f }, // bottom-left
        { -0.5f,  0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f }, // top-left
                                                                // Front face
        { -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f }, // bottom-left
        {  0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f }, // bottom-right
        {  0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f }, // top-right
        {  0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f }, // top-right
        { -0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f }, // top-left
        { -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f }, // bottom-left
                                                               // Left face
        { -0.5f,  0.5f,  0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f }, // top-right
        { -0.5f,  0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f }, // top-left
        { -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f }, // bottom-left
        { -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f }, // bottom-left
        { -0.5f, -0.5f,  0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f }, // bottom-right
        { -0.5f,  0.5f,  0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f }, // top-right
                                                                // Right face
        { 0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f },   // top-left
        { 0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f },   // bottom-right
        { 0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f },   // top-right         
        { 0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f },   // bottom-right
        { 0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f },   // top-left
        { 0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f },   // bottom-left     
                                                                // Bottom face
        { -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f }, // top-right
        {  0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f }, // top-left
        {  0.5f, -0.5f,  0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f }, // bottom-left
        {  0.5f, -0.5f,  0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f }, // bottom-left
        { -0.5f, -0.5f,  0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f }, // bottom-right
        { -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f }, // top-right
                                                                // Top face
        { -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f },  // top-left
        {  0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f },  // bottom-right
        {  0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f },  // top-right     
        {  0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f },  // bottom-right
        { -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f },  // top-left
        { -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f }   // bottom-left 
    };

    const uint32_t indices[] = {
         0,  1,  2,
         3,  4,  5,
         6,  7,  8,
         9, 10, 11,
        12, 13, 14,
        15, 16, 17,
        18, 19, 20,
        21, 22, 23,
        24, 25, 26,
        27, 28, 29,
        30, 31, 32,
        33, 34, 35
    };

    BufferLayout layout;
    layout.push_attribute("a_position", 3, GL_FLOAT, 4);
    layout.push_attribute("a_normal", 3, GL_FLOAT, 4);
    layout.push_attribute("a_tex_coord", 2, GL_FLOAT, 4);

    VertexArray vao;
    vao.add_vertex_buffer(vertices, ARRAY_COUNT(vertices) * sizeof(CubeVertex), ArrayBufferUsage::STATIC, layout);
    vao.add_index_buffer(indices, ARRAY_COUNT(indices), ArrayBufferUsage::STATIC);
    vao.apply_vertex_attributes();
    return vao;
}

void draw_chunk(Chunk &chunk, const Shader &shader) {
    static bool        s_initialized = false;
    static VertexArray s_cube_vao = gen_cube_vao();
    static Texture     s_sand_tex("C://dev//emce//data//sand.png");
    static float       s_elapsed = 0.0f;

    if(!s_initialized) {
        s_initialized = true;
        s_sand_tex.set_filter_min(GL_NEAREST);
        s_sand_tex.set_filter_mag(GL_NEAREST);
    }

    shader.use_program();
    shader.upload_int("u_texture", 0);
    s_sand_tex.bind_texture_unit(0);
    s_cube_vao.bind_vao();

    for_every_block(x, y, z) {
        Block &block = chunk.get_block(x, y, z);

        /* Don't draw AIR */
        if(block.is_of_type(BlockType::AIR)) {
            continue;
        }

        mat4 model = mat4::translate({ float(x), float(y), float(z) });
        shader.upload_mat4("u_model", model.e);
        glDrawElements(GL_TRIANGLES, s_cube_vao.index_count(), GL_UNSIGNED_INT, 0);
    }
}

int SDL_main(int argc, char *argv[]) {
    /* TEMP: init std randomizer */
    srand(time(NULL));

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

    /* Initialize OpenGL state */ {
        GL_CHECK(glEnable(GL_DEPTH_TEST));
        GL_CHECK(glDepthFunc(GL_LESS));
    }

    Camera camera;
    camera.set_position({ 5.0f, 20.0f, 5.0f });
    camera.set_rotation({ DEG_TO_RAD(90.0f), 0.0f });

    ShaderFile shader("C://dev//emce//source//shaders//block.glsl");

    Chunk chunk;

    const double time_freq = SDL_GetPerformanceFrequency();
    uint64_t time_now  = SDL_GetPerformanceCounter();
    uint64_t time_last = SDL_GetPerformanceCounter();
    double delta_time  = 0.0;

    while(window.is_running()) {
        window.process_events(input);

        shader.hotload();

        if(input.key_pressed(Key::ESCAPE)) {
            window.should_quit();
        }

        /* Calculate time delta */ {
            time_now = SDL_GetPerformanceCounter();
            const double delta = time_now - time_last;
            delta_time = delta / time_freq;
            time_last = time_now;
        }

        /* Update camera */ {
            int32_t move_fw   = 0;
            int32_t move_side = 0;

            if(input.key_is_down(Key::W)) { move_fw += 1; }
            if(input.key_is_down(Key::S)) { move_fw -= 1; }
            if(input.key_is_down(Key::A)) { move_side -= 1; }
            if(input.key_is_down(Key::D)) { move_side += 1; }

            int32_t rotate_h = 0;
            int32_t rotate_v = 0;

#if 0
            if(input.key_is_down(Key::UP))    { rotate_v += 10; }
            if(input.key_is_down(Key::DOWN))  { rotate_v -= 10; }
            if(input.key_is_down(Key::LEFT))  { rotate_h -= 10; }
            if(input.key_is_down(Key::RIGHT)) { rotate_h += 10; }
#else
            if(input.button_is_down(Button::LEFT)) {
                rotate_h =  input.mouse_rel_x();
                rotate_v = -input.mouse_rel_y();
            }
#endif

            camera.update_free(move_fw, move_side, rotate_v, rotate_h, delta_time, input.key_is_down(Key::LEFT_SHIFT));
        }

        mat4 proj_m = camera.calc_proj((float)window.width() / (float)window.height());
        mat4 view_m = camera.calc_view();

        glViewport(0, 0, window.width(), window.height());

        GL_CHECK(glClearColor(0.2f, 0.2f, 0.4f, 1.0));
        GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

        shader.use_program();
        shader.upload_mat4("u_proj", proj_m.e);
        shader.upload_mat4("u_view", view_m.e);

        draw_chunk(chunk, shader);

        window.swap_buffers();
    }

    return 0;
}
