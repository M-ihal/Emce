#include "simple_draw.h"

#include "vertex_array.h"
#include "shader.h"

#include <glew.h>

namespace {
    bool        g_initialized = false;
    ShaderFile  g_cube_outline_shader;
    VertexArray g_cube_outline_vao;
    ShaderFile  g_line_shader;
    VertexArray g_line_vao;
    float       g_set_aspect = 1.0f;
    Camera      g_set_camera;
}

void SimpleDraw::initialize(void) {
    ASSERT(!g_initialized, "SimpleDraw already initialized.\n");
    g_initialized = true;

    g_cube_outline_shader.set_filepath_and_load("C://dev//emce//source//shaders//sd_outline_cube.glsl");
    g_line_shader.set_filepath_and_load("C://dev//emce//source//shaders//sd_line.glsl");

    /* Cube outline */ {
        const float vertices[] = {
            0, 0, 0,
            1, 0, 0,
            1, 1, 0,
            0, 1, 0,
            0, 0, 1,
            1, 0, 1,
            1, 1, 1,
            0, 1, 1 
        };

        const uint32_t indices[] = {
            0, 1, 2,
            2, 3, 0,
            4, 5, 6,
            6, 7, 4,
            0, 3, 7,
            7, 4, 0,
            1, 5, 6,
            6, 2, 1,
            0, 1, 5,
            5, 4, 0,
            3, 2, 6,
            6, 7, 3
        };

        BufferLayout layout;
        layout.push_attribute("a_position", 3, BufferDataType::FLOAT);

        g_cube_outline_vao.create_vao(layout, ArrayBufferUsage::STATIC);
        g_cube_outline_vao.set_vbo_data(vertices, sizeof(float) * ARRAY_COUNT(vertices));
        g_cube_outline_vao.set_ibo_data(indices, ARRAY_COUNT(indices));
        g_cube_outline_vao.apply_vao_attributes();
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
    g_line_shader.hotload();
}

void SimpleDraw::set_camera(const Camera &camera, float aspect_ratio) {
    g_set_aspect = aspect_ratio;
    g_set_camera = camera;
}

void SimpleDraw::draw_cube_outline(const vec3 &position, const vec3 &size, float width, const Color &color) {
    mat4 view = g_set_camera.calc_view();
    mat4 proj = g_set_camera.calc_proj(g_set_aspect);
    mat4 model = mat4::scale(size.x, size.y, size.z);
    model = model * mat4::translate(position);

    g_cube_outline_shader.use_program();
    g_cube_outline_shader.upload_mat4("u_view", view.e);
    g_cube_outline_shader.upload_mat4("u_proj", proj.e);
    g_cube_outline_shader.upload_mat4("u_model", model.e);
    g_cube_outline_shader.upload_vec3("u_color", (float *)color.e);
    g_cube_outline_shader.upload_vec3("u_size", (float *)size.e);
    g_cube_outline_shader.upload_float("u_width", width);

    g_cube_outline_vao.bind_vao();

    GL_CHECK(glDrawElements(GL_TRIANGLES, g_cube_outline_vao.get_ibo_count(), GL_UNSIGNED_INT, NULL));
}

void SimpleDraw::draw_line(const vec3 &point_a, const vec3 &point_b, float width, const Color &color) {
    mat4 view = g_set_camera.calc_view();
    mat4 proj = g_set_camera.calc_proj(g_set_aspect);

    float vertices[] = {
        point_a.x, point_a.y, point_a.z,
        point_b.x, point_b.y, point_b.z
    };

    const int32_t vbo_data_size = g_line_vao.get_vbo_size();
    ASSERT(vbo_data_size == ARRAY_COUNT(vertices) * sizeof(float));

    g_line_shader.use_program();
    g_line_shader.upload_mat4("u_view", view.e);
    g_line_shader.upload_mat4("u_proj", proj.e);
    g_line_shader.upload_vec3("u_color", (float *)color.e);

    g_line_vao.bind_vao();
    g_line_vao.upload_vbo_data(vertices, vbo_data_size, 0);

    GL_CHECK(glLineWidth(width));
    GL_CHECK(glDrawElements(GL_LINES, g_line_vao.get_ibo_count(), GL_UNSIGNED_INT, NULL));
}
