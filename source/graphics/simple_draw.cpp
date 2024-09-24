#include "simple_draw.h"

#include "vertex_array.h"
#include "shader.h"

#include <glew.h>

namespace {
    bool s_initialized = false;

    ShaderFile  s_cube_outline_shader;
    VertexArray s_cube_outline_vao;

    float  s_set_aspect = 1.0f;
    Camera s_set_camera;
}

void SimpleDraw::initialize(void) {
    ASSERT(!s_initialized, "SimpleDraw already initialized.\n");
    s_initialized = true;

    s_cube_outline_shader.set_filepath_and_load("C://dev//emce//source//shaders//sd_outline_cube.glsl");

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

    s_cube_outline_vao.create_vao(layout, ArrayBufferUsage::STATIC);
    s_cube_outline_vao.set_vbo_data(vertices, sizeof(float) * ARRAY_COUNT(vertices));
    s_cube_outline_vao.set_ibo_data(indices, ARRAY_COUNT(indices));
    s_cube_outline_vao.apply_vao_attributes();
}

void SimpleDraw::destroy(void) {
    ASSERT(s_initialized, "SimpleDraw was not initialized\n");
    s_initialized = false;

    s_cube_outline_shader.delete_shader_file();
    s_cube_outline_vao.delete_vao();
}

void SimpleDraw::hotload_shader(void) {
    s_cube_outline_shader.hotload();
}

void SimpleDraw::set_camera(const Camera &camera, float aspect_ratio) {
    s_set_aspect = aspect_ratio;
    s_set_camera = camera;
}

void SimpleDraw::draw_cube_outline(const vec3 &position, const vec3 &size, float width, const Color &color) {
    mat4 view = s_set_camera.calc_view();
    mat4 proj = s_set_camera.calc_proj(s_set_aspect);
    mat4 model = mat4::scale(size.x, size.y, size.z);
    model = model * mat4::translate(position);

    s_cube_outline_shader.use_program();
    s_cube_outline_shader.upload_mat4("u_view", view.e);
    s_cube_outline_shader.upload_mat4("u_proj", proj.e);
    s_cube_outline_shader.upload_mat4("u_model", model.e);
    s_cube_outline_shader.upload_vec3("u_color", (float *)color.e);
    s_cube_outline_shader.upload_vec3("u_size", (float *)size.e);
    s_cube_outline_shader.upload_float("u_width", width);

    s_cube_outline_vao.bind_vao();

    GL_CHECK(glDrawElements(GL_TRIANGLES, s_cube_outline_vao.get_ibo_count(), GL_UNSIGNED_INT, NULL));
}

