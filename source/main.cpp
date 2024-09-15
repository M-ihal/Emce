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

/*
    @todo: Game / GameManager / Game-Something class
    @todo: Better cleanup of reasorces... maybe make it explicit not in destructors
*/

// @todo Batch text rendering 
void test_render_text(const Shader &shader, const VertexArray &glyph_vao, const Font &font, int32_t window_w, int32_t window_h) {
    const char *text = "TEST!";
    vec2 cursor = { 100.0f, 100.0f };
    const size_t len = strlen(text);

    shader.use_program();
    shader.upload_mat4("u_proj", mat4::orthographic(0.0f, 0.0f, float(window_w), float(window_h), -1.0f, 1.0f).e);
    shader.upload_vec3("u_color", vec3{ 1.0f, 1.0f, 1.0f }.e);
    shader.upload_int("u_font_atlas", 0);
    font.get_atlas().bind_texture_unit(0);  

    for(int32_t index = 0; index < len; ++index) {
        const int32_t codepoint = text[index];

        Font::Glyph glyph;
        if(!font.get_glyph(codepoint, glyph)) continue; // @todo has_glyph

        vec2 glyph_size = { float(glyph.width), float(glyph.height) };
        vec2 glyph_p = {
            cursor.x + glyph.left_side_bearing * font.get_scale_for_pixel_height(),
            cursor.y - glyph.height - glyph.offset_y
        };

        float verts[] = {
            glyph_p.x,                glyph_p.y, glyph.tex_coords[0].x, glyph.tex_coords[0].y,
            glyph_p.x + glyph_size.x, glyph_p.y, glyph.tex_coords[1].x, glyph.tex_coords[1].y,
            glyph_p.x + glyph_size.x, glyph_p.y + glyph_size.y, glyph.tex_coords[2].x, glyph.tex_coords[2].y,
            glyph_p.x,                glyph_p.y + glyph_size.y, glyph.tex_coords[3].x, glyph.tex_coords[3].y,

        };

        glyph_vao.upload_vbo_data(verts, 4 * 4 * sizeof(float), 0);

        glyph_vao.bind_vao();
        glDrawElements(GL_TRIANGLES, glyph_vao.get_ibo_count(), GL_UNSIGNED_INT, NULL);

        // const int32_t adv = glyph->advance + font->kerning_advance(string[idx], string[idx + 1]);
        // cursor.x += adv * font->scale_for_pixel_height;
        cursor.x += float(glyph.width);
    }
}

void render_random_thing_at_origin(const Camera &camera, float aspect) {
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
    camera.set_position({ 0.0f, 128.0f, -5.0f });
    camera.set_rotation({ DEG_TO_RAD(90.0f), DEG_TO_RAD(-45.0f) });

    ShaderFile shader("C://dev//emce//source//shaders//block.glsl");

    ShaderFile text_shader("C://dev//emce//source//shaders//simple_text.glsl");

    VertexArray glyph_vao;
    /* Initialzie text vao */ {
        BufferLayout layout;
        layout.push_attribute("a_position", 2, BufferDataType::FLOAT);
        layout.push_attribute("a_tex_coord", 2, BufferDataType::FLOAT);
        glyph_vao.create_vao(layout, ArrayBufferUsage::STATIC);
        glyph_vao.set_vbo_data(NULL, 4 * 4 * sizeof(float));
        const uint32_t inds[] = { 0, 1, 2, 2, 3, 0 };
        glyph_vao.set_ibo_data(inds, 6);
        glyph_vao.apply_vao_attributes();
    }

    Texture sand_texture;
    sand_texture.load_from_file("C://dev//emce//data//sand.png");
    sand_texture.set_filter_min(TextureFilter::NEAREST);
    sand_texture.set_filter_mag(TextureFilter::NEAREST);

    World world;

    Font font;
    font.load_from_ttf_file("C://dev//emce//data//font.ttf", 16);

    const double time_freq = SDL_GetPerformanceFrequency();
    uint64_t time_now  = SDL_GetPerformanceCounter();
    uint64_t time_last = SDL_GetPerformanceCounter();
    double delta_time  = 0.0;

    while(window.is_running()) {
        window.process_events(input);

        shader.hotload();
        text_shader.hotload();

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

            if(input.button_is_down(Button::LEFT)) {
                rotate_h =  input.mouse_rel_x();
                rotate_v = -input.mouse_rel_y();
            }

            camera.update_free(move_fw, move_side, rotate_v, rotate_h, delta_time, input.key_is_down(Key::LEFT_SHIFT));
        }

        float aspect_ratio = (float)window.width() / (float)window.height();

        mat4 proj_m = camera.calc_proj(aspect_ratio);
        mat4 view_m = camera.calc_view();

        glViewport(0, 0, window.width(), window.height());

        GL_CHECK(glClearColor(0.2f, 0.2f, 0.4f, 1.0));
        GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

        shader.use_program();
        shader.upload_mat4("u_proj", proj_m.e);
        shader.upload_mat4("u_view", view_m.e);

        world.render_chunks(shader, sand_texture);

        render_random_thing_at_origin(camera, aspect_ratio);

        test_render_text(text_shader, glyph_vao, font, window.width(), window.height());

        window.swap_buffers();
    }

    sand_texture.delete_texture_if_exists();
    font.delete_font();

    return 0;
}
