#include "game_renderer.h"
#include "game.h"
#include "opengl_abs.h"

#include <stb_image.h>
#include <vector>
#include <queue>
#include <algorithm>

static int32_t calc_fps(double delta_time);

namespace {
    static int32_t g_triangles_rendered_this_frame = 0;
};

/* Upload block pixels from atlas to _created_ TextureArray */
void init_block_texture_array(TextureArray &texture_array, const char *atlas_filepath);

/* Upload water pixels from image to TextureArray */
void init_water_texture_array(TextureArray &texture_array, const char *water_filepath);

GameRenderer::GameRenderer(int32_t width, int32_t height) {
    m_width = width;
    m_height = height;
    m_aspect = (double)width / (double)height;

    m_fog_color = vec3{ 0.4f, 0.5f, 0.8f };

    /* Initialize framebuffers */ {
        FboConfig fbo_ms_config;
        fbo_ms_config.ms_samples = 16;
        fbo_config_push_color(fbo_ms_config, { .target = FboAttachmentTarget::TEXTURE_MULTISAMPLE, .format = TextureDataFormat::RGBA });
        fbo_config_push_color(fbo_ms_config, { .target = FboAttachmentTarget::TEXTURE_MULTISAMPLE, .format = TextureDataFormat::RGBA });
        fbo_config_push_color(fbo_ms_config, { .target = FboAttachmentTarget::TEXTURE_MULTISAMPLE, .format = TextureDataFormat::RGBA });
        fbo_config_push_color(fbo_ms_config, { .target = FboAttachmentTarget::TEXTURE_MULTISAMPLE, .format = TextureDataFormat::RGBA });
        fbo_config_set_depth(fbo_ms_config,  { .target = FboAttachmentTarget::TEXTURE_MULTISAMPLE, .format = TextureDataFormat::DEPTH24_STENCIL8 });
        m_fbo_ms.create_fbo(m_width, m_height, fbo_ms_config);

        FboConfig fbo_config;
        fbo_config_push_color(fbo_config, { .target = FboAttachmentTarget::TEXTURE, .format = TextureDataFormat::RGBA });
        fbo_config_push_color(fbo_config, { .target = FboAttachmentTarget::TEXTURE, .format = TextureDataFormat::RGBA });
        fbo_config_push_color(fbo_config, { .target = FboAttachmentTarget::TEXTURE, .format = TextureDataFormat::RGBA });
        fbo_config_push_color(fbo_config, { .target = FboAttachmentTarget::TEXTURE, .format = TextureDataFormat::RGBA });
        fbo_config_set_depth(fbo_config,  { .target = FboAttachmentTarget::TEXTURE, .format = TextureDataFormat::DEPTH24_STENCIL8 });
        m_fbo.create_fbo(m_width, m_height, fbo_config);
    }

    /* Load resources */ {
        m_ui_font.load_from_file("data//MinecraftRegular-Bmg3.otf", 20);
        m_ui_font_big.load_from_file("data//MinecraftRegular-Bmg3.otf", 40);
        m_ui_font_smooth.load_from_file("data//CascadiaMono.ttf", 16);

        m_chunk_shader.set_filepath_and_load("source//shaders//chunk.glsl");
        m_chunk_tex_array.load_empty_reserve(16, 16, 64, TextureDataFormat::RGBA, 4);

        m_water_shader.set_filepath_and_load("source//shaders//water.glsl");
        m_water_tex_array.load_empty_reserve(16, 16, 32, TextureDataFormat::RGBA, 4);

        init_block_texture_array(m_chunk_tex_array, "data//atlas.png");
        init_water_texture_array(m_water_tex_array, "data//water.png");

        m_chunk_tex_array.gen_mipmaps();
        m_water_tex_array.gen_mipmaps();

        m_chunk_tex_array.set_filter_min(TextureFilter::NEAREST_MIPMAP_LINEAR);
        m_water_tex_array.set_filter_min(TextureFilter::NEAREST_MIPMAP_LINEAR);
    }

    /* init skybox */ {
        m_skybox_clean.load_from_file({
            "data//skybox_clean//right.png",
            "data//skybox_clean//left.png",
            "data//skybox_clean//up.png",
            "data//skybox_clean//down.png",
            "data//skybox_clean//front.png",
            "data//skybox_clean//back.png",
        });
        m_skybox_clean.set_filter_min(TextureFilter::LINEAR);
        m_skybox_clean.set_filter_mag(TextureFilter::LINEAR);

        m_skybox_cloudy.load_from_file({
            "data//skybox_cloudy//right.bmp",
            "data//skybox_cloudy//left.bmp",
            "data//skybox_cloudy//up.bmp",
            "data//skybox_cloudy//down.bmp",
            "data//skybox_cloudy//front.bmp",
            "data//skybox_cloudy//back.bmp",
        });
        m_skybox_cloudy.set_filter_min(TextureFilter::LINEAR);
        m_skybox_cloudy.set_filter_mag(TextureFilter::LINEAR);

        m_skybox_weird.load_from_file({
            "data//skybox_weird//right.jpg",
            "data//skybox_weird//left.jpg",
            "data//skybox_weird//up.jpg",
            "data//skybox_weird//down.jpg",
            "data//skybox_weird//front.jpg",
            "data//skybox_weird//back.jpg",
        });
        m_skybox_weird.set_filter_min(TextureFilter::LINEAR);
        m_skybox_weird.set_filter_mag(TextureFilter::LINEAR);

        m_skybox_shader.set_filepath_and_load("source//shaders//skybox.glsl");

        const float sv_vertices[] = {
            -0.5f, -0.5f,  0.5f,
            0.5f, -0.5f,  0.5f,
            0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
            -0.5f, -0.5f, -0.5f,
            0.5f, -0.5f, -0.5f,
            0.5f,  0.5f, -0.5f,
            -0.5f,  0.5f, -0.5f 
        };

        const uint32_t sv_indices[] = {
            0, 1, 2,
            2, 3, 0,
            4, 5, 6,
            6, 7, 4,
            4, 0, 3,
            3, 7, 4,
            1, 5, 6,
            6, 2, 1,
            3, 2, 6,
            6, 7, 3,
            0, 1, 5,
            5, 4, 0
        };

        BufferLayout sv_layout;
        sv_layout.push_attribute("a_position", 3, BufferDataType::FLOAT);

        m_skybox_vao.create_vao(sv_layout, ArrayBufferUsage::STATIC);
        m_skybox_vao.apply_vao_attributes();
        m_skybox_vao.set_vbo_data(sv_vertices, ARRAY_COUNT(sv_vertices) * sizeof(float));
        m_skybox_vao.set_ibo_data(sv_indices, ARRAY_COUNT(sv_indices));
    }

    /* Init single block vao with temp data */ {
        BufferLayout layout;
        layout.push_attribute("a_packed", 2, BufferDataType::UINT);

        const uint32_t num_vertices = 6 * 4;
        const uint32_t num_indices  = 6 * 6;

        m_block_vao.create_vao(layout, ArrayBufferUsage::STATIC);
        m_block_vao.set_vbo_data(NULL,  num_vertices * sizeof(ChunkVaoVertexPacked));
        m_block_vao.set_ibo_data(NULL,  num_indices);
        m_block_vao.apply_vao_attributes();
    }

    /* Init post process rect vao and shader */ {
        m_post_process_shader.set_filepath_and_load("source//shaders//post_process.glsl");

        BufferLayout rect_layout;
        rect_layout.push_attribute("a_position", 2, BufferDataType::FLOAT);
        rect_layout.push_attribute("a_tex_coords", 2, BufferDataType::FLOAT);

        float rect_vertices[4 * 4] = {
            0.0f, 0.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
            0.0f, 1.0f, 0.0f, 1.0f
        };

        uint32_t rect_indices[2 * 3] = {
            0, 1, 2,
            2, 3, 0
        };

        m_post_process_vao.create_vao(rect_layout, ArrayBufferUsage::STATIC);
        m_post_process_vao.set_vbo_data(rect_vertices, 4 * 4 * sizeof(float));
        m_post_process_vao.set_ibo_data(rect_indices, 2 * 3);
        m_post_process_vao.apply_vao_attributes();
    }

    /* Init crosshair rendering resources */ {
        BufferLayout layout;
        layout.push_attribute("a_position", 2, BufferDataType::FLOAT);
        layout.push_attribute("a_coords", 2, BufferDataType::FLOAT);

        float verts[] = {
            0.0f, 0.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
            0.0f, 1.0f, 0.0f, 1.0f,
        };

        uint32_t inds[] = { 0, 1, 2, 2, 3, 0 };

        m_crosshair_shader.set_filepath_and_load("source//shaders//crosshair.glsl");
        m_crosshair_vao.create_vao(layout, ArrayBufferUsage::STATIC);
        m_crosshair_vao.set_vbo_data(verts, ARRAY_COUNT(verts) * sizeof(float));
        m_crosshair_vao.set_ibo_data(inds, ARRAY_COUNT(inds));
        m_crosshair_vao.apply_vao_attributes();
    }

    /* Cube outline */ {
        m_cube_outline_shader.set_filepath_and_load("source//shaders//cube_outline.glsl");

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

        m_cube_outline_vao.create_vao(layout, ArrayBufferUsage::STATIC);
        m_cube_outline_vao.set_vbo_data(cube_vertices, sizeof(float) * ARRAY_COUNT(cube_vertices));
        m_cube_outline_vao.set_ibo_data(indices, ARRAY_COUNT(indices));
        m_cube_outline_vao.apply_vao_attributes();
    }

    /* Cube line outline */ {
        m_cube_line_outline_shader.set_filepath_and_load("source//shaders//cube_line_outline.glsl");

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

        m_cube_line_outline_vao.create_vao(layout, ArrayBufferUsage::STATIC);
        m_cube_line_outline_vao.set_vbo_data(cube_vertices, sizeof(float) * ARRAY_COUNT(cube_vertices));
        m_cube_line_outline_vao.set_ibo_data(indices, ARRAY_COUNT(indices));
        m_cube_line_outline_vao.apply_vao_attributes();
    }

    /* Line */ {
        m_line_shader.set_filepath_and_load("source//shaders//line.glsl");

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

        m_line_vao.create_vao(layout, ArrayBufferUsage::STATIC);
        m_line_vao.set_vbo_data(vertices, sizeof(float) * ARRAY_COUNT(vertices));
        m_line_vao.set_ibo_data(indices, ARRAY_COUNT(indices));
        m_line_vao.apply_vao_attributes();
    }
}

GameRenderer::~GameRenderer(void) {
    m_fbo_ms.delete_fbo();
    m_fbo.delete_fbo();

    m_ui_font.delete_font();
    m_ui_font_big.delete_font();
    m_ui_font_smooth.delete_font();

    m_chunk_shader.delete_shader_file();
    m_chunk_tex_array.delete_texture_array();

    m_water_shader.delete_shader_file();
    m_water_tex_array.delete_texture_array();

    m_skybox_vao.delete_vao();
    m_skybox_shader.delete_shader_file();
    m_skybox_clean.delete_cubemap();
    m_skybox_cloudy.delete_cubemap();
    m_skybox_weird.delete_cubemap();

    m_post_process_shader.delete_shader_file();
    m_post_process_vao.delete_vao();

    m_crosshair_shader.delete_shader_file();
    m_crosshair_vao.delete_vao();

    m_block_vao.delete_vao();
}

void GameRenderer::resize(int32_t width, int32_t height) {
    m_width = width;
    m_height = height;
    m_aspect = (double)width / (double)height;
    m_fbo_ms.resize_fbo(width, height);
    m_fbo.resize_fbo(width, height);
}

void GameRenderer::hotload_shaders(void) {
    m_chunk_shader.hotload();
    m_water_shader.hotload();
    m_skybox_shader.hotload();
    m_post_process_shader.hotload();
    m_crosshair_shader.hotload();
}

void GameRenderer::render_frame(Game &game) {
    g_triangles_rendered_this_frame = 0;

    set_viewport({ 0, 0, m_width, m_height });

    /* Clear multisampled framebuffer */
    m_fbo_ms.clear_all({ 0.1f, 0.1f, 0.1f, 1.0f });
    m_fbo_ms.bind_fbo();

    m_camera = game.get_camera();
    m_proj_m = m_camera.calc_proj(m_aspect);
    m_view_m = m_camera.calc_view_at_origin();
    m_camera.calc_frustum_at_origin(m_frustum, m_aspect);

    // Render world to multisampled framebuffer
    this->render_world(game);

    this->render_target_block(game);

    this->render_held_block(game);

    /* Resolve multisampled framebuffer to normal framebuffer */
    m_fbo.clear_all({ 0.1f, 0.1f, 0.1f, 1.0f });
    m_fbo_ms.blit_color_attachment(0, m_fbo, 0);
    m_fbo_ms.blit_color_attachment(1, m_fbo, 1);
    m_fbo_ms.blit_color_attachment(2, m_fbo, 2);
    m_fbo_ms.blit_color_attachment(3, m_fbo, 3);
    m_fbo_ms.blit_depth_attachment(m_fbo);

    World  &world = game.get_world();
    Player &player = world.get_player();

    /* Blit multisampled framebuffer to default framebuffer */
    switch(game.render_mode) {
        case GameRenderMode::NORMAL: {
            set_render_state({
                .blend = BlendFunc::DISABLE,
                .depth = DepthFunc::DISABLE,
                .multisample = false,
            });

            int32_t is_camera_in_water = 0;

            /* Check if camera is in water */ {
                const vec3d head_position = player.get_head_camera().get_position();
                const vec3i head_block = WorldPosition::from_real(head_position).block;
                BlockType block = world.get_block(head_block);
                if(block == BlockType::WATER) {
                    is_camera_in_water = 1;
                }
            }

            m_post_process_shader.use_program();
            m_post_process_shader.upload_mat4("u_proj", mat4::orthographic(0.0f, 0.0f, 1.0f, 1.0f, -1.0f, 1.0f));
            m_post_process_shader.upload_int("u_texture", 0);
            m_post_process_shader.upload_int("u_in_water", is_camera_in_water);
            m_post_process_shader.upload_int("u_texture", 0);

            m_fbo.bind_color_texture_unit(0, 0);

            bind_no_fbo();
            m_post_process_vao.bind_vao();
            draw_elements_triangles(m_post_process_vao.get_ibo_count());
        } break;

        case GameRenderMode::DEBUG_NORMALS: {
            m_fbo.blit_color_attachment(1, m_width, m_height);
        } break;

        case GameRenderMode::DEBUG_DEPTH: {
            m_fbo.blit_color_attachment(2, m_width, m_height);
        } break;

        case GameRenderMode::DEBUG_AO: {
            m_fbo.blit_color_attachment(3, m_width, m_height);
        } break;
    }

    /* Bind default framebuffer and render UI */
    bind_no_fbo();

    this->render_crosshair(game);

    if(game.debug_show_ui_info) {
        this->render_debug_ui(game);
    }

    /* Render held block text */ {
        m_batcher.begin();
        const vec2 text_pos = vec2{ 6.0f, -m_ui_font_big.get_descent() * m_ui_font_big.get_scale_for_pixel_height() };
        m_batcher.push_text_formatted(text_pos, m_ui_font_big, "Held block: %s", block_type_string[(int32_t)world.get_player().get_held_block()]);
        m_batcher.render(m_width, m_height, m_ui_font_big, { 1.0f, 1.0f, 1.0f }, { 4, -4 });
    }

    Console &console = game.get_console();
    console.render(m_width, m_height);
}

void GameRenderer::render_world(Game &game) {
    World  &world = game.get_world();
    Player &player = world.get_player();
    GameFrameInfo info = game.get_frame_info();

    m_chunk_shader.use_program();
    m_chunk_shader.upload_mat4("u_proj", m_proj_m);
    m_chunk_shader.upload_mat4("u_view", m_view_m);
    m_chunk_shader.upload_int("u_load_radius", info.load_radius);
    m_chunk_shader.upload_int("u_texture_array", 0);
    m_chunk_shader.upload_int("u_skybox", 1);
    m_chunk_shader.upload_int("u_fog_enable", game.fog_enable == true ? 1 : 0);
    m_chunk_shader.upload_vec3("u_fog_color", m_fog_color.e);
    m_chunk_shader.upload_float("u_plane_near", m_camera.get_plane_near());
    m_chunk_shader.upload_float("u_plane_far", m_camera.get_plane_far());

    m_chunk_tex_array.bind_texture_unit(0);
    this->get_skybox_cubemap(game.skybox_choice).bind_cubemap_unit(1);

    set_render_state({
        .blend = BlendFunc::DISABLE,
        .depth = DepthFunc::LESS,
        .multisample = true,
        .cull_faces = true
    });

    if(game.debug_chunk_wireframe_mode) {
        set_line_width(1.0f);
        set_polygon_mode(true);
    }

    std::vector<Chunk *> water_chunks;

    /* Render chunks and get chunks with water to render */
    ChunkHashTable::Iterator chunk_iter;
    while(world.iterate_chunks(chunk_iter)) {
        Chunk *chunk = chunk_iter.value;

        vec2i chunk_coords = chunk->get_chunk_coords();
        vec3d chunk_absolute = real_position_from_chunk(chunk_coords);
        vec3d chunk_relative = m_camera.offset_to_relative(chunk_absolute);

        const bool chunk_in_frustum = is_chunk_in_frustum(m_frustum, chunk_relative);
        if(!chunk_in_frustum) {
            continue;
        }

        /* Gather water chunks */
        const VertexArray &water_vao = chunk->get_water_vao();
        const bool has_water_vao = water_vao.has_been_created() && water_vao.get_ibo_count();
        if(has_water_vao) {
            water_chunks.push_back(chunk);
        }

        const VertexArray &chunk_vao = chunk->get_vao();
        const bool chunk_vao_valid = chunk_vao.has_been_created() && chunk_vao.get_ibo_count();
        if(!chunk_vao_valid) {
            continue;
        }

        vec3d chunk_render_absolute = chunk_absolute;
        // chunk_render_absolute.y = chunk->get_chunk_render_offset_y();
        vec3 chunk_render_offset = vec3::make(m_camera.offset_to_relative(chunk_render_absolute));

        m_chunk_shader.upload_mat4("u_model", mat4::translate(chunk_render_offset));
        chunk_vao.bind_vao();
        draw_elements_triangles(chunk_vao.get_ibo_count());

        g_triangles_rendered_this_frame += chunk_vao.get_ibo_count() / 3;
    }

    if(game.debug_chunk_wireframe_mode) {
        set_polygon_mode(false);
    }

    this->render_player(game);

    this->render_skybox(game);

    /* DEBUG render chunk borders */
    if(game.debug_show_chunk_borders) {
        ChunkHashTable::Iterator iter;
        while(world.iterate_chunks(iter)) {
            Chunk *chunk = iter.value;
            vec3d chunk_pos = { 
                (double)(chunk->get_chunk_coords().x * CHUNK_SIZE_X),
                0.0f,
                (double)(chunk->get_chunk_coords().y * CHUNK_SIZE_Z)
            };
            this->render_cube_outline(chunk_pos, { CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z }, 8.0f / 16.0f, { 1.0f, 1.0f, 1.0f }, 0.2f, { 0.0f, 0.0f, 0.0f });
        }
    }

    /* Sort water chunks, @TODO : TODO */
    std::sort(water_chunks.begin(), water_chunks.end(), [] (Chunk *a, Chunk *b) { 
        vec2 chunk = vec2::make(a->get_world()->get_player().get_position_chunk());
        float dist_a = vec2::length_sq(chunk - vec2::make(a->get_chunk_coords()));
        float dist_b = vec2::length_sq(chunk - vec2::make(b->get_chunk_coords()));
        return dist_a > dist_b; 
    });

    set_render_state({
        .blend = BlendFunc::STANDARD,
        .depth = DepthFunc::LESS,
        .multisample = true,
        .cull_faces = true,
        .disable_depth_write = true
    });

    m_water_shader.use_program();
    m_water_shader.upload_mat4("u_proj", m_proj_m);
    m_water_shader.upload_mat4("u_view", m_view_m);
    m_water_shader.upload_float("u_time_elapsed", info.elapsed_time);
    m_water_shader.upload_int("u_load_radius", info.load_radius);
    m_water_shader.upload_int("u_fog_enable", game.fog_enable == true ? 1 : 0);
    m_water_shader.upload_vec3("u_fog_color", m_fog_color.e);
    m_water_shader.upload_int("u_water_texture_array", 0);
    m_water_shader.upload_int("u_skybox", 1);
    m_water_shader.upload_float("u_plane_near", m_camera.get_plane_near());
    m_water_shader.upload_float("u_plane_far", m_camera.get_plane_far());
    m_water_tex_array.bind_texture_unit(0);
    this->get_skybox_cubemap(game.skybox_choice).bind_cubemap_unit(1);

    /* Render water */
    for(auto &chunk : water_chunks) {
        vec2i chunk_coords = chunk->get_chunk_coords();
        vec3d chunk_absolute = real_position_from_chunk(chunk_coords);
        vec3d chunk_relative = m_camera.offset_to_relative(chunk_absolute);

        vec3d chunk_render_absolute = chunk_absolute;
        // chunk_render_absolute.y = chunk->get_chunk_render_offset_y();
        vec3 chunk_render_offset = vec3::make(m_camera.offset_to_relative(chunk_render_absolute));

        m_water_shader.upload_mat4("u_model", mat4::translate(chunk_render_offset));

        const VertexArray &water_vao = chunk->get_water_vao();
        water_vao.bind_vao();
        draw_elements_triangles(water_vao.get_ibo_count());

        g_triangles_rendered_this_frame += water_vao.get_ibo_count() / 3;
    }
}

void GameRenderer::render_player(Game &game) {
    set_render_state({
        .blend = BlendFunc::DISABLE,
        .depth = DepthFunc::LESS,
        .multisample = true,
        .cull_faces = false
    });

    World  &world = game.get_world();
    Player &player = world.get_player();

    /* Render shape in third person mode */
    if(game.get_frame_info().is_3rd_person) {
        const vec3 color = { 0.7f, 0.9f, 0.6f };
        const vec3d position = player.get_position_origin();
        const vec3d size = player.get_collider_size();
        // SimpleDraw::draw_cube_outline(position, size, 2.0f / 32.0f, color);
        this->render_cube_outline(position, size, 2.0f / 32.0f, color);
    }

    /* Render player's debug stuff */
    if(game.debug_player_draw) {
        this->render_player_debug(game);
    }
}

void GameRenderer::render_player_debug(Game &game) {
    World  &world = game.get_world();
    Player &player = world.get_player();

    /* Collider */
    const vec3 collider_color = { 0.9f, 0.9f, 0.6f };
    const vec3d collider_position = player.get_position_origin();
    const vec3d collider_size = player.get_collider_size();
    this->render_cube_outline(collider_position, collider_size, 1.0f / 32.0f, collider_color);

    /* Ground check collider */ {
        vec3 ground_collider_color = player.is_grounded() ? vec3{ 0.0f, 1.0f, 0.0f } : vec3{ 1.0f, 0.0f, 0.0f };
        vec3d ground_collider_position;
        vec3d ground_collider_size;
        player.get_ground_collider_info(ground_collider_position, ground_collider_size);
        this->render_cube_outline(ground_collider_position, ground_collider_size, 1.0f / 32.0f, ground_collider_color);
    }

    /* Velocity lines */ {
        const vec3d velocity = player.get_velocity();
        const vec3d origin_position = player.get_position() + vec3d{ 0, player.get_collider_size().y * 0.5, 0 };
        const vec3d xz_velocity = { velocity.x, 0.0, velocity.z };
        const vec3d y_velocity  = { 0.0, velocity.y, 0.0 };

        this->render_line(origin_position, origin_position + xz_velocity, 2.0f, { 1.0f, 0.0f, 1.0f });
        this->render_line(origin_position, origin_position + y_velocity, 2.0f, { 0.0f, 1.0f, 0.0f });
    }

    /* Blocks checked when checking collisions */
    const vec3d min_pos = real_position_from_block(player.debug_min_checked_block);
    const vec3d max_pos = real_position_from_block(player.debug_max_checked_block) + vec3d::make(1.0);
    this->render_cube_outline(min_pos, max_pos - min_pos, 0.05f, { 0.8f, 0.5f, 0.9f }); 
}

void GameRenderer::render_skybox(Game &game) {
    set_render_state({
        .blend = BlendFunc::DISABLE,
        .depth = DepthFunc::LESS_OR_EQUAL,
        .multisample = true
    });

    m_skybox_shader.use_program();
    m_skybox_shader.upload_mat4("u_proj", m_proj_m);
    m_skybox_shader.upload_mat4("u_view", m_view_m);
    m_skybox_vao.bind_vao();
    this->get_skybox_cubemap(game.skybox_choice).bind_cubemap();
    draw_elements_triangles(m_skybox_vao.get_ibo_count());
}

void GameRenderer::render_held_block(Game &game) {
    World  &world = game.get_world();
    Player &player = world.get_player();

    double elapsed_time = game.get_frame_info().elapsed_time;

    const Camera &head_cam = m_camera; // player.get_head_camera();
    const double exp_rot = cosf(elapsed_time * 0.8f) * 0.1f;
    const double exp_pos = sinf(elapsed_time * 1.2f) * 0.035f;

    const vec3d vec_up = head_cam.calc_direction_up();
    const vec3d vec_fw = head_cam.calc_direction();
    const vec3d vec_rt = head_cam.calc_direction_side();

    double rotate_y = head_cam.get_rotation().x;
    double rotate_z = head_cam.get_rotation().y;

    auto ease_out_quint = [] (double x) -> double {
        return 1 - pow(1.0 - x, 5);
    };

    double perc = player.get_hand_anim_time() / BLOCK_ACTION_ANIM_TIME;

#if 0
    perc = 1.0 - perc;
    perc *= 0.8;
    const double anim_perc = ((cos(perc * M_PI * 1.0) - 1.0) * (sin(perc * M_PI * 2.5))) * 0.3; //M_PI * 0.5 * ease_out_quint(1.0 - perc);
    rotate_z -= anim_perc;
#else
    const double anim_perc = M_PI * 0.5 * ease_out_quint(1.0 - perc);
    rotate_z -= sin(anim_perc * M_PI) * 0.12;
#endif

    const vec3d scale = { 0.16f, 0.16f, 0.16f };
    const vec3d position = vec_fw * 0.4 + vec_rt * 0.155 * m_aspect - vec_up * (0.28 + exp_pos);

    mat4 transform;
    transform = mat4::scale(scale);        
    transform *= mat4::rotate_z(-rotate_z);
    transform *= mat4::rotate_y(rotate_y);
    transform *= mat4::translate(position);


    set_render_state({
        .blend = BlendFunc::STANDARD,
        .depth = DepthFunc::ALWAYS,
        .multisample = true,
        .cull_faces = true
    });

    BlockType held_block = player.get_held_block();

    // @TODO : do not do this every frame
    ChunkMeshData mesh_data = { };
    chunk_mesh_gen_single_block(mesh_data, held_block);

    m_block_vao.set_vbo_data(mesh_data.vertices.data(), mesh_data.vertices.size() * sizeof(ChunkVaoVertexPacked));
    m_block_vao.set_ibo_data(mesh_data.indices.data(), mesh_data.indices.size());
    m_block_vao.apply_vao_attributes();

    m_chunk_shader.use_program();
    m_chunk_shader.upload_mat4("u_proj", m_proj_m);
    m_chunk_shader.upload_mat4("u_view", m_view_m);
    m_chunk_shader.upload_mat4("u_model", transform);
    m_chunk_shader.upload_int("u_load_radius", -1);
    m_chunk_shader.upload_int("u_fog_enable", 0);
    m_chunk_shader.upload_vec3("u_fog_color", m_fog_color.e);
    m_chunk_shader.upload_int("u_texture_array", 0);
    m_chunk_shader.upload_int("u_skybox", 1);
    m_chunk_shader.upload_float("u_plane_near", m_camera.get_plane_near());
    m_chunk_shader.upload_float("u_plane_far", m_camera.get_plane_far());
    m_chunk_tex_array.bind_texture_unit(0);
    this->get_skybox_cubemap(game.skybox_choice).bind_cubemap_unit(1);

    m_block_vao.bind_vao();
    draw_elements_triangles(m_block_vao.get_ibo_count());
}

void GameRenderer::render_target_block(Game &game) {
    World &world = game.get_world();
    RaycastBlockResult target = world.get_player().get_targeted_block();
    if(!target.found) {
        return;
    }

    set_render_state({
        .blend = BlendFunc::DISABLE,
        .depth = DepthFunc::DISABLE,
        .multisample = true,
    });

    /* Target block outline */
    set_line_width(2.0f);

    /* Line target block outline */ {
        const vec3d block_position = target.block_p.real;
        this->render_cube_line_outline(block_position, vec3d::make(1), { 0.0f, 0.0f, 0.0f });
    }

    /* Raycasted side's normal */
    const vec3d next_position = WorldPosition::from_block(target.block_p.block + target.normal).real;
    const vec3d block_position = target.block_p.real;
    const vec3  line_color = vec3::absolute(vec3::make(get_block_side_normal(target.side)));
    this->render_line(block_position + vec3d::make(0.5) + vec3d::make(target.normal) * vec3d::make(0.5), next_position + vec3d::make(0.5), 2.0f, line_color);

    if(game.debug_raycast_draw) {
        /* DEBUG: Itersection point */
        const vec3d point_size_half = vec3d::make(0.05);
        const vec3d intersect_position = target.intersection;
        set_line_width(2.0f);
        this->render_cube_line_outline(intersect_position - point_size_half, point_size_half * 2.0, { 0.6f, 0.75f, 0.4f });

    }
}

void GameRenderer::render_crosshair(Game &game) {
    set_render_state({
        .blend = BlendFunc::STANDARD,
        .depth = DepthFunc::DISABLE,
        .multisample = true,
        .cull_faces = false
    });

    const int32_t crosshair_size  = 18;
    const int32_t crosshair_width = 2;

    const vec2i position = {
        m_width  / 2 - crosshair_size / 2,
        m_height / 2 - crosshair_size / 2
    };

    mat4 proj_m = mat4::orthographic(0.0f, 0.0f, m_width, m_height, -1.0f, 1.0f);
    mat4 model_m = mat4::scale(crosshair_size, crosshair_size, 1.0f) * mat4::translate(position.x, position.y, 0.0f);

    m_crosshair_shader.use_program();
    m_crosshair_shader.upload_mat4("u_proj", proj_m);
    m_crosshair_shader.upload_mat4("u_model", model_m);
    m_crosshair_shader.upload_float("u_perc", (float)crosshair_width / (float)crosshair_size);

    m_crosshair_vao.bind_vao();
    draw_elements_triangles(m_crosshair_vao.get_ibo_count());
}

void GameRenderer::render_debug_ui(Game &game) {
    Font &font = m_ui_font_smooth;

    const float spacing  = 2.0f;
    const vec2  padding  = { 4.0f, 4.0f };
    const float baseline = m_height - padding.y - font.get_ascent() * font.get_scale_for_pixel_height();

    float cursor_x = padding.x;
    float cursor_y = baseline;

    m_batcher.begin();

    /* Pushes next line to the batcher */
    auto debug_line = [&] (const char *format, ...) {
        if(format != NULL) {
            va_list args;
            va_start(args, format);
            m_batcher.push_text_va_args({ cursor_x, cursor_y }, font, format, args);
            va_end(args);
        }
        cursor_y -= font.get_height() + spacing;
    };

    World  &world  = game.get_world();
    Player &player = world.get_player();
    GameFrameInfo info = game.get_frame_info();

    debug_line("Build: %s", BUILD_TYPE);
    debug_line(NULL);

    debug_line("--- Frame ---"); {
        

        debug_line("fps: %d", calc_fps(info.delta_time));
        debug_line("delta time:   %f", info.delta_time);
        debug_line("elapsed time: %f", info.elapsed_time);
    } debug_line(NULL);

    debug_line("--- Render ---"); {
        debug_line("triangles rendered: %u (%.3fmln)", g_triangles_rendered_this_frame, (double)g_triangles_rendered_this_frame / 1000000.0);
    } debug_line(NULL);

    debug_line("--- Player ---"); {
        WorldPosition position = WorldPosition::from_real(player.get_position());
        const vec3d player_orientation = player.get_head_camera().calc_direction();

        debug_line("real:      %+.2f, %+.2f, %+.2f", position.real.x, position.real.y, position.real.z);
        debug_line("block:     %+02d, %+02d, %+02d", position.block.x, position.block.y, position.block.z);
        debug_line("block rel: %+02d, %+02d, %+02d", position.block_rel.x, position.block_rel.y, position.block_rel.z);
        debug_line("chunk:     %+02d, %+02d", position.chunk.x, position.chunk.y);
        debug_line("orientation %+.3f, %+.3f, %+.3f", player_orientation.x, player_orientation.y, player_orientation.z);
    } debug_line(NULL);

    debug_line("--- Camera ---"); {
        vec3d cam_dir = m_camera.calc_direction();
        vec3d cam_pos = m_camera.get_position();

        debug_line("position:  %+.3f, %+.3f, %+.3f", cam_pos.x, cam_pos.y, cam_pos.z);
        debug_line("direction: %+.3f, %+.3f, %+.3f", cam_dir.x, cam_dir.y, cam_dir.z);
    } debug_line(NULL);

    debug_line("--- World ---"); {
        GameWorldInfo world_info = world.get_world_info();

        debug_line("chunk size: %d, %d, %d", CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z);
        debug_line("world seed: %u", world_info.world_gen_seed);
        debug_line("load radius: %d", info.load_radius);
        debug_line("loaded chunks: %u", world_info.chunks_loaded);
        debug_line("chunk buckets: %u", world_info.chunks_allocated);
        debug_line("gen chunks threads: %d", info.chunks_threads_active);
        debug_line("gen meshes threads: %d", info.meshes_threads_active);
        debug_line("chunks to generate: %d", world_info.chunks_queued);
        debug_line("meshes to build:    %d", world_info.meshes_queued);
        debug_line(" \\--> high prio:    %d", world_info.meshes_queued_high_prio);
    } debug_line(NULL);

    debug_line("--- Target Block ---"); {
        RaycastBlockResult target_block = player.get_targeted_block();
        BlockType targeted_block = world.get_block(target_block.block_p.block);

        if(target_block.found) {
            debug_line("block: %s", block_type_string[(uint32_t)targeted_block]);
            debug_line("block abs: %+02d, %+02d, %+02d", target_block.block_p.block.x, target_block.block_p.block.y, target_block.block_p.block.z);
        } else {
            debug_line("None");
        }
    } debug_line(NULL);

    /* Render debug list */
    m_batcher.render(m_width, m_height, font, { 1.0f, 1.0f, 1.0f }, { 4, -4 });
}

void GameRenderer::render_cube_outline(const vec3d &position, const vec3d &size, float width, const vec3 &color, float border_perc, const vec3 &border_color) {
    const vec3 position_relative = vec3::make(m_camera.offset_to_relative(position));

    mat4 model = mat4::scale(size.x, size.y, size.z) * mat4::translate(position_relative);

    const vec3 size_f = vec3::make(size);

    m_cube_outline_shader.use_program();
    m_cube_outline_shader.upload_mat4("u_view", m_view_m);
    m_cube_outline_shader.upload_mat4("u_proj", m_proj_m);
    m_cube_outline_shader.upload_mat4("u_model", model);
    m_cube_outline_shader.upload_vec3("u_color", (float *)color.e);
    m_cube_outline_shader.upload_vec3("u_size", (float *)size_f.e);
    m_cube_outline_shader.upload_float("u_width", width);
    m_cube_outline_shader.upload_float("u_border_perc", border_perc);
    m_cube_outline_shader.upload_vec3("u_border_color", (float *)border_color.e);
 
    m_cube_outline_vao.bind_vao();
    draw_elements_triangles(m_cube_outline_vao.get_ibo_count());
}

void GameRenderer::render_cube_line_outline(const vec3d &position, const vec3d &size, const vec3 &color) {
    const vec3 position_relative = vec3::make(m_camera.offset_to_relative(position));

    mat4 model = mat4::scale(size.x, size.y, size.z) * mat4::translate(position_relative);

    m_cube_line_outline_shader.use_program();
    m_cube_line_outline_shader.upload_mat4("u_view", m_view_m);
    m_cube_line_outline_shader.upload_mat4("u_proj", m_proj_m);
    m_cube_line_outline_shader.upload_mat4("u_model", model);
    m_cube_line_outline_shader.upload_vec3("u_color", (float *)color.e);

    m_cube_line_outline_vao.bind_vao();
    draw_elements_lines(m_cube_line_outline_vao.get_ibo_count());
}

void GameRenderer::render_line(const vec3d &point_a, const vec3d &point_b, float width, const vec3 &color) {
    const vec3 point_a_relative = vec3::make(m_camera.offset_to_relative(point_a));
    const vec3 point_b_relative = vec3::make(m_camera.offset_to_relative(point_b));

    float vertices[] = {
        point_a_relative.x, point_a_relative.y, point_a_relative.z,
        point_b_relative.x, point_b_relative.y, point_b_relative.z
    };

    const int32_t vbo_data_size = m_line_vao.get_vbo_size();
    ASSERT(vbo_data_size == ARRAY_COUNT(vertices) * sizeof(float));

    m_line_shader.use_program();
    m_line_shader.upload_mat4("u_view", m_view_m);
    m_line_shader.upload_mat4("u_proj", m_proj_m);
    m_line_shader.upload_vec3("u_color", (float *)color.e);

    m_line_vao.bind_vao();
    m_line_vao.upload_vbo_data(vertices, vbo_data_size, 0);

    set_line_width(width);

    draw_elements_lines(m_line_vao.get_ibo_count());
}

Cubemap &GameRenderer::get_skybox_cubemap(GameSkyboxChoice choice) {
    switch(choice) {
        default: INVALID_CODE_PATH; return m_skybox_clean;
        case GameSkyboxChoice::CLEAN: return m_skybox_clean;
        case GameSkyboxChoice::CLOUDY: return m_skybox_cloudy;
        case GameSkyboxChoice::WEIRD: return m_skybox_weird;
    }
}

static int32_t calc_fps(double delta_time) {
    static int32_t draw_fps = 0;
    static int32_t frame_times_counter = 0;
    static double  frame_times[32] = { };

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

    return draw_fps;
}

void init_block_texture_array(TextureArray &texture_array, const char *atlas_filepath) {
    stbi_set_flip_vertically_on_load(true);

    int32_t width;
    int32_t height;
    int32_t channels;
    uint8_t *pixels = (uint8_t *)stbi_load(atlas_filepath, &width, &height, &channels, 4);

    if(pixels) {
        // @todo
        ASSERT(channels == 4); 

        uint8_t *rect = (uint8_t *)malloc(16 * 16 * 4);
        ASSERT(rect);

        for(uint8_t texture = 0; texture < (uint8_t)BlockTexture::_COUNT; ++texture) {
            if(texture >= texture_array.get_count()) {
                fprintf(stderr, "[Error] Not enough texture array slots to fill block textures.\n");
                break;
            }

            const vec2i texture_tile = get_block_texture_tile((BlockTexture)texture);
            const uint32_t rect_x = texture_tile.x * 16;
            const uint32_t rect_y = texture_tile.y * 16;

            rect = copy_image_memory_rect(pixels, width, height, channels, rect_x, rect_y, 16, 16, rect);
            texture_array.set_pixels(texture, rect, 16, 16, TextureDataFormat::RGBA, TextureDataType::UNSIGNED_BYTE);
        }

        free(rect);
        free(pixels);
    }
}

void init_water_texture_array(TextureArray &texture_array, const char *water_filepath) {
    stbi_set_flip_vertically_on_load(true);

    int32_t width;
    int32_t height;
    int32_t channels;
    uint8_t *pixels = (uint8_t *)stbi_load(water_filepath, &width, &height, &channels, 4);

    if(pixels) {
        /* Assert that there are 8 water frames in the image */
        uint32_t water_frames = 32;
        ASSERT(water_frames == (height / 16));

        // @todo
        ASSERT(channels == 4); 

        uint8_t *rect = (uint8_t *)malloc(16 * 16 * 4);
        ASSERT(rect);

        for(int32_t water_frame = 0; water_frame < water_frames; ++water_frame) {
            const uint32_t rect_x = 0;
            const uint32_t rect_y = water_frame * 16;

            rect = copy_image_memory_rect(pixels, width, height, channels, rect_x, rect_y, 16, 16, rect);
            texture_array.set_pixels(water_frame, rect, 16, 16, TextureDataFormat::RGBA, TextureDataType::UNSIGNED_BYTE);
        }

        free(rect);
        free(pixels);
    }
}
