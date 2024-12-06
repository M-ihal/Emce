#include "simple_draw.h"

#include "texture.h"
#include "vertex_array.h"
#include "shader.h"
#include "opengl_abs.h"

namespace {
    bool        g_initialized = false;
    ShaderFile  g_cube_outline_shader;
    VertexArray g_cube_outline_vao;
    ShaderFile  g_cube_line_outline_shader;
    VertexArray g_cube_line_outline_vao;
    ShaderFile  g_line_shader;
    VertexArray g_line_vao;
    double      g_set_aspect = 1.0;
    Camera      g_set_camera;
}

void SimpleDraw::initialize(void) {
    ASSERT(!g_initialized);
    g_initialized = true;

    g_cube_outline_shader.set_filepath_and_load("source//shaders//cube_outline.glsl");
    g_cube_line_outline_shader.set_filepath_and_load("source//shaders//cube_line_outline.glsl");
    g_line_shader.set_filepath_and_load("source//shaders//line.glsl");

    const float cube_vertices[] = {
        0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 1.0f,

        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
    };

    /* Cube outline */ {
        const uint32_t indices[] = {
            0, 1, 2,
            2, 3, 0,

            1, 5, 6,
            6, 2, 1,

            5, 4, 7,
            7, 6, 5,

            4, 0, 3,
            3, 7, 4,

            4, 5, 1,
            1, 0, 4,

            3, 2, 6,
            6, 7, 3
        };

        BufferLayout layout;
        layout.push_attribute("a_position", 3, BufferDataType::FLOAT);

        g_cube_outline_vao.create_vao(layout, ArrayBufferUsage::STATIC);
        g_cube_outline_vao.set_vbo_data(cube_vertices, sizeof(float) * ARRAY_COUNT(cube_vertices));
        g_cube_outline_vao.set_ibo_data(indices, ARRAY_COUNT(indices));
        g_cube_outline_vao.apply_vao_attributes();
    }

    /* Cube line outline */ {
        const uint32_t indices[] = {
            0, 1,
            1, 2,
            2, 3,
            3, 0,
            
            1, 5,
            5, 6,
            6, 2,
            2, 1,

            5, 4,
            4, 7,
            7, 6,
            6, 5,

            4, 0,
            0, 3,
            3, 7,
            7, 4,

            4, 5,
            5, 1,
            1, 0,
            0, 4,

            3, 2,
            2, 6,
            6, 7,
            7, 3
        };

        BufferLayout layout;
        layout.push_attribute("a_position", 3, BufferDataType::FLOAT);

        g_cube_line_outline_vao.create_vao(layout, ArrayBufferUsage::STATIC);
        g_cube_line_outline_vao.set_vbo_data(cube_vertices, sizeof(float) * ARRAY_COUNT(cube_vertices));
        g_cube_line_outline_vao.set_ibo_data(indices, ARRAY_COUNT(indices));
        g_cube_line_outline_vao.apply_vao_attributes();
    }

    /* Line */ {
        /* Stub vertices */
        const float vertices[] = {
            0, 0, 0,
            1, 1, 1
        };

        const uint32_t indices[] = {
            0, 1
        };

        BufferLayout layout;
        layout.push_attribute("a_position", 3, BufferDataType::FLOAT);

        g_line_vao.create_vao(layout, ArrayBufferUsage::STATIC);
        g_line_vao.set_vbo_data(vertices, sizeof(float) * ARRAY_COUNT(vertices));
        g_line_vao.set_ibo_data(indices, ARRAY_COUNT(indices));
        g_line_vao.apply_vao_attributes();
    }
}

void SimpleDraw::destroy(void) {
    ASSERT(g_initialized, "SimpleDraw was not initialized\n");
    g_initialized = false;

    g_cube_outline_shader.delete_shader_file();
    g_cube_outline_vao.delete_vao();
    g_line_shader.delete_shader_file();
    g_line_vao.delete_vao();
}

void SimpleDraw::hotload_shader(void) {
    g_cube_outline_shader.hotload();
    g_cube_line_outline_shader.hotload();
    g_line_shader.hotload();
}

void SimpleDraw::set_camera(const Camera &camera, double aspect_ratio) {
    g_set_aspect = aspect_ratio;
    g_set_camera = camera;
}

void SimpleDraw::draw_cube_line_outline(const vec3d &position, const vec3d &size, const vec3 &color) {
    const vec3 position_relative = vec3::make(g_set_camera.offset_to_relative(position));

    mat4 view = g_set_camera.calc_view_at_origin();
    mat4 proj = g_set_camera.calc_proj(g_set_aspect);
    mat4 model = mat4::scale(size.x, size.y, size.z) * mat4::translate(position_relative);

    g_cube_line_outline_shader.use_program();
    g_cube_line_outline_shader.upload_mat4("u_view", view);
    g_cube_line_outline_shader.upload_mat4("u_proj", proj);
    g_cube_line_outline_shader.upload_mat4("u_model", model);
    g_cube_line_outline_shader.upload_vec3("u_color", (float *)color.e);

    g_cube_line_outline_vao.bind_vao();
    draw_elements_lines(g_cube_line_outline_vao.get_ibo_count());
}

void SimpleDraw::draw_cube_outline(const vec3d &position, const vec3d &size, float width, const vec3 &color, float border_perc, const vec3 &border_color) {
    const vec3 position_relative = vec3::make(g_set_camera.offset_to_relative(position));

    mat4 view = g_set_camera.calc_view_at_origin();
    mat4 proj = g_set_camera.calc_proj(g_set_aspect);
    mat4 model = mat4::scale(size.x, size.y, size.z) * mat4::translate(position_relative);

    const vec3 size_f = vec3::make(size);

    g_cube_outline_shader.use_program();
    g_cube_outline_shader.upload_mat4("u_view", view);
    g_cube_outline_shader.upload_mat4("u_proj", proj);
    g_cube_outline_shader.upload_mat4("u_model", model);
    g_cube_outline_shader.upload_vec3("u_color", (float *)color.e);
    g_cube_outline_shader.upload_vec3("u_size", (float *)size_f.e);
    g_cube_outline_shader.upload_float("u_width", width);
    g_cube_outline_shader.upload_float("u_border_perc", border_perc);
    g_cube_outline_shader.upload_vec3("u_border_color", (float *)border_color.e);
 
    g_cube_outline_vao.bind_vao();
    draw_elements_triangles(g_cube_outline_vao.get_ibo_count());
}

void SimpleDraw::draw_line(const vec3d &point_a, const vec3d &point_b, float width, const vec3 &color) {
    const vec3 point_a_relative = vec3::make(g_set_camera.offset_to_relative(point_a));
    const vec3 point_b_relative = vec3::make(g_set_camera.offset_to_relative(point_b));

    mat4 view = g_set_camera.calc_view_at_origin();
    mat4 proj = g_set_camera.calc_proj(g_set_aspect);

    float vertices[] = {
        point_a_relative.x, point_a_relative.y, point_a_relative.z,
        point_b_relative.x, point_b_relative.y, point_b_relative.z
    };

    const int32_t vbo_data_size = g_line_vao.get_vbo_size();
    ASSERT(vbo_data_size == ARRAY_COUNT(vertices) * sizeof(float));

    g_line_shader.use_program();
    g_line_shader.upload_mat4("u_view", view);
    g_line_shader.upload_mat4("u_proj", proj);
    g_line_shader.upload_vec3("u_color", (float *)color.e);

    g_line_vao.bind_vao();
    g_line_vao.upload_vbo_data(vertices, vbo_data_size, 0);

    set_line_width(width);

    draw_elements_lines(g_line_vao.get_ibo_count());
}
