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
#include "world.h"
#include "font.h"
#include "text_batcher.h"
#include "debug_ui.h"
#include "console.h"

#define INIT_WINDOW_WIDTH  1280
#define INIT_WINDOW_HEIGHT 720
#define INIT_WINDOW_TITLE  "emce"

/*
    @todo: Game / GameManager / Game-Something class
    @todo: Better cleanup of reasorces... maybe make it explicit not in destructors
    @todo: Maybe make window globally accesible
*/

vec3i block_position_from_real(const vec3 &real) {
    return {
        (int32_t)floorf(real.x),
        (int32_t)floorf(real.y),
        (int32_t)floorf(real.z),
    };
}

vec2i chunk_position_from_block(const vec3i &block) {
    return {
        (int32_t)floorf(float(block.x) / CHUNK_SIZE_X),
        (int32_t)floorf(float(block.z) / CHUNK_SIZE_Z)
    };
}

struct WorldPosition {
    static WorldPosition from_real(const vec3 &real);
    vec3  real;
    vec3i block;
    vec3i block_rel;
    vec2i chunk;
};

WorldPosition WorldPosition::from_real(const vec3 &real) {
    WorldPosition result = { };
    result.real = real;
    result.block = block_position_from_real(real);
//    result.block_rel = block_relative_from_block(result.block)
    result.chunk = chunk_position_from_block(result.block);
    return result;
}

void render_random_thing_at_origin(const Camera &camera, float aspect) {
    /// @todo NOT DELETING THOSE....
    static Shader s_shader;
    static VertexArray s_vao;
    static Texture s_texture;
    static bool    s_initialized = false;
    if(!s_initialized) {
        s_initialized = true;
        s_shader.load_from_file("C://dev//emce//source//shaders//flat_image.glsl");
        s_texture.load_from_file("C://dev//emce//data//dog.png", true);

        const float fval = 4.0f;

        const float vertices[] = {
            -fval, -fval, 0.0f, 0.0f, 0.0f,
            +fval, -fval, 0.0f, 1.0f, 0.0f,
            +fval, +fval, 0.0f, 1.0f, 1.0f,
            -fval, +fval, 0.0f, 0.0f, 1.0f,
        };

        const uint32_t indices[] = {
            0, 1, 2,
            2, 3, 0
        };

        BufferLayout layout;
        layout.push_attribute("a_position", 3, BufferDataType::FLOAT);
        layout.push_attribute("a_tex_coord", 2, BufferDataType::FLOAT);

        s_vao.create_vao(layout, ArrayBufferUsage::STATIC);
        s_vao.set_vbo_data(vertices, ARRAY_COUNT(vertices) * sizeof(float));
        s_vao.set_ibo_data(indices, 6);
        s_vao.apply_vao_attributes();
    }

    mat4 proj_m = camera.calc_proj(aspect);
    mat4 view_m = camera.calc_view();
    s_shader.use_program();
    s_shader.upload_mat4("u_proj", proj_m.e);
    s_shader.upload_mat4("u_view", view_m.e);
    s_shader.upload_mat4("u_model", mat4::translate({ 0.0f, 130.0f, 0.0f }).e);
    s_shader.upload_int("u_image", 0);
    s_texture.bind_texture_unit(0);

    s_vao.bind_vao();
    glDrawElements(GL_TRIANGLES, s_vao.get_ibo_count(), GL_UNSIGNED_INT, NULL);
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
    
    if(!window.initialize(INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT, INIT_WINDOW_TITLE)) {
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

    TextBatcher::initialize();
    DebugUI::initialize();
    Console::initialize();

    Camera camera;
    camera.set_position({ -54.0f, 128.0f, 37.0f });
    camera.set_rotation({ DEG_TO_RAD(90.0f), DEG_TO_RAD(-45.0f) });

    ShaderFile shader;
    shader.set_filepath_and_load("C://dev//emce//source//shaders//block.glsl");

    Texture sand_texture;
    sand_texture.load_from_file("C://dev//emce//data//sand.png");
    sand_texture.set_filter_min(TextureFilter::NEAREST);
    sand_texture.set_filter_mag(TextureFilter::NEAREST);

    Font font;
    font.load_from_ttf_file("C://dev//emce//data//CascadiaMono.ttf", 16);
    font.get_atlas().set_filter_min(TextureFilter::NEAREST);
    font.get_atlas().set_filter_mag(TextureFilter::NEAREST);

    World world;
    
    for(int32_t x = -8; x <= 8; ++x) 
        for(int32_t z = -8; z <= 8; ++z) 
            world.gen_chunk_at({ x, z });

    const double time_freq = SDL_GetPerformanceFrequency();
    uint64_t time_now  = SDL_GetPerformanceCounter();
    uint64_t time_last = SDL_GetPerformanceCounter();
    double elapsed_time = 0.0;
    double delta_time   = 0.0;

    while(window.is_running()) {
        window.process_events(input);

        DebugUI::begin_frame(window.width(), window.height());

        TextBatcher::hotload_shader();
        shader.hotload();

        if(input.key_pressed(Key::ESCAPE)) {
            window.should_quit();
        }

        /* Calculate time delta */ {
            time_now = SDL_GetPerformanceCounter();
            const double delta = time_now - time_last;
            delta_time = delta / time_freq;
            elapsed_time += delta_time;
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

            if(input.button_is_down(Button::LEFT)) {
                rotate_h =  input.mouse_rel_x();
                rotate_v = -input.mouse_rel_y();
            }

            camera.update_free(move_fw, move_side, rotate_v, rotate_h, delta_time, input.key_is_down(Key::LEFT_SHIFT));
        }

        Console::update(input);

        float aspect_ratio = (float)window.width() / (float)window.height();

        mat4 proj_m = camera.calc_proj(aspect_ratio);
        mat4 view_m = camera.calc_view();

        glViewport(0, 0, window.width(), window.height());

        GL_CHECK(glClearColor(0.2f, 0.2f, 0.4f, 1.0));
        GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

        shader.use_program();
        shader.upload_mat4("u_proj", proj_m.e);
        shader.upload_mat4("u_view", view_m.e);

        WorldPosition position = WorldPosition::from_real(camera.get_position());

        world.render_chunks(shader, sand_texture);

        render_random_thing_at_origin(camera, aspect_ratio);

        Console::render(window.width(), window.height());

        /* Draw debug info */ {
            static int32_t draw_fps = 0;
            static int32_t frame_times_counter = 0;
            static double frame_times[32] = { };
            frame_times_counter++;
            frame_times_counter %= ARRAY_COUNT(frame_times);
            frame_times[frame_times_counter] = delta_time;
            double average = 0.0f;
            for(int32_t i = 0; i < ARRAY_COUNT(frame_times); ++i) {
                average += frame_times[i];
            }
            const int32_t num_frames = ARRAY_COUNT(frame_times);
            average /= double(num_frames);
            int32_t fps = int32_t(1.0 / average);
            if(frame_times_counter == ARRAY_COUNT(frame_times) - 1) {
                draw_fps = fps;
            }

            DebugUI::push_text_left(" --- Frame ---");
            DebugUI::push_text_left("elapsed time: %.3f", elapsed_time);
            DebugUI::push_text_left("frame time: %.6f", delta_time);
            DebugUI::push_text_left("fps: %d (%d frames sampled)", draw_fps, ARRAY_COUNT(frame_times));
            DebugUI::push_text_left(NULL);

            DebugUI::push_text_left(" --- Position ---");
            DebugUI::push_text_left("real:     %+.2f, %+.2f, %+.2f", position.real.x, position.real.y, position.real.z);
            DebugUI::push_text_left("block:    %+d, %+d, %+d", position.block.x, position.block.y, position.block.z);
            DebugUI::push_text_left("blockrel: %+d, %+d, %+d", position.block_rel.x, position.block_rel.y, position.block_rel.z);
            DebugUI::push_text_left("chunk:    %+d, %+d", position.chunk.x, position.chunk.y);
            DebugUI::push_text_left("rotation H: %+.3f", RAD_TO_DEG(camera.get_rotation().x));
            DebugUI::push_text_left("rotation V: %+.3f", RAD_TO_DEG(camera.get_rotation().y));

        }

        DebugUI::render();
        
        window.swap_buffers();
    }

    sand_texture.delete_texture_if_exists();
    font.delete_font();
    shader.delete_shader_file();

    Console::destroy();
    DebugUI::destroy();
    TextBatcher::destroy();

    return 0;
}
