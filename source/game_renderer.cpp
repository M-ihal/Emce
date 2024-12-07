#include "game_renderer.h"
#include "game.h"
#include "opengl_abs.h"
#include "simple_draw.h"

static int32_t calc_fps(double delta_time);

namespace {
    static int32_t g_triangles_rendered_this_frame = 0;
};

GameRenderer::GameRenderer(int32_t width, int32_t height) {
    m_width = width;
    m_height = height;
    m_aspect = (double)width / (double)height;

    /* Initialize framebuffers */ {
        FboConfig fbo_ms_config;
        fbo_ms_config.ms_samples = 16; // Window::get().get_samples();
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
        const std::string folder = "skybox_blue";
        const std::string extension = ".jpg";

        

        const std::string filepaths[6] = {
            "data//" + folder + "//right" + extension,
            "data//" + folder + "//left" + extension,
            "data//" + folder + "//up" + extension,
            "data//" + folder + "//down" + extension,
            "data//" + folder + "//front" + extension,
            "data//" + folder + "//back" + extension,
        };

        m_skybox_cubemap.load_from_file(filepaths);
        m_skybox_cubemap.set_filter_min(TextureFilter::LINEAR);
        m_skybox_cubemap.set_filter_mag(TextureFilter::LINEAR);

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
    m_skybox_cubemap.delete_cubemap();

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

    Camera &camera = game.get_camera();
    SimpleDraw::set_camera(camera, m_aspect);

    mat4 proj_m = camera.calc_proj(m_aspect);
    mat4 view_m = camera.calc_view_at_origin();

    // Render world to multisampled framebuffer


    /* Bind default framebuffer and render UI */
    bind_no_fbo();

    this->render_world(game, view_m, proj_m);

    this->render_skybox(game, view_m, proj_m);

    this->render_held_block(game, view_m, proj_m);

    this->render_target_block(game, view_m, proj_m);

    this->render_crosshair(game);

    if(game.debug_show_ui_info) {
        this->render_debug_ui(game);
    }

    Console &console = game.get_console();
    console.render(m_width, m_height);
}

void GameRenderer::render_world(Game &game, const mat4 &view_m, const mat4 &proj_m) {
    World  &world = game.get_world();
    Player &player = world.get_player();
    Camera &camera = game.get_camera();

    m_chunk_shader.use_program();
    m_chunk_shader.upload_mat4("u_proj", proj_m);
    m_chunk_shader.upload_mat4("u_view", view_m);
    m_chunk_shader.upload_int("u_load_radius", game.get_load_radius());
    m_chunk_shader.upload_int("u_texture_array", 0);
    m_chunk_shader.upload_int("u_skybox", 1);
    m_chunk_tex_array.bind_texture_unit(0);
    m_skybox_cubemap.bind_cubemap_unit(1);

    set_render_state({
        .blend = BlendFunc::DISABLE,
        .depth = DepthFunc::LESS,
        .multisample = true,
        .cull_faces = true
    });

    double frustum[6][4];
    camera.calc_frustum_at_origin(frustum, m_aspect);

    if(game.debug_chunk_wireframe_mode) {
        set_line_width(1.0f);
        set_polygon_mode(true);
    }

    ChunkHashTable chunk_table = world.get_chunk_table();
    ChunkHashTable::Iterator chunk_iter;

    while(chunk_table.iterate_all(chunk_iter)) {
        Chunk *chunk = chunk_iter.value;

        const bool chunk_valid = chunk && chunk->get_chunk_state() == ChunkState::GENERATED && chunk->get_mesh_state() == ChunkMeshState::LOADED;
        if(!chunk_valid) {
            continue;
        }
    
        vec2i chunk_coords = chunk->get_chunk_xz();
        vec3d chunk_absolute = real_position_from_chunk(chunk_coords);
        vec3d chunk_relative = camera.offset_to_relative(chunk_absolute);

        const bool chunk_in_frustum = is_chunk_in_frustum(frustum, chunk_relative);
        if(!chunk_in_frustum) {
            continue;
        }

        const VertexArray &chunk_vao = chunk->get_vao();
        const bool chunk_vao_valid = chunk_vao.has_been_created() && chunk_vao.get_ibo_count();
        if(!chunk_vao_valid) {
            continue;
        }

        vec3d chunk_render_absolute = chunk_absolute;
        chunk_render_absolute.y = chunk->get_chunk_render_offset_y();
        vec3 chunk_render_offset = vec3::make(camera.offset_to_relative(chunk_render_absolute));

        m_chunk_shader.upload_mat4("u_model", mat4::translate(chunk_render_offset));
        chunk_vao.bind_vao();
        draw_elements_triangles(chunk_vao.get_ibo_count());

        g_triangles_rendered_this_frame += chunk_vao.get_ibo_count() / 3;
    }

    if(game.debug_chunk_wireframe_mode) {
        set_polygon_mode(false);
    }
}

void GameRenderer::render_skybox(Game &game, const mat4 &view_m, const mat4 &proj_m) {
    set_render_state({
        .blend = BlendFunc::DISABLE,
        .depth = DepthFunc::LESS_OR_EQUAL,
        .multisample = true
    });

    m_skybox_shader.use_program();
    m_skybox_shader.upload_mat4("u_proj", proj_m);
    m_skybox_shader.upload_mat4("u_view", view_m);
    m_skybox_vao.bind_vao();
    m_skybox_cubemap.bind_cubemap();
    draw_elements_triangles(m_skybox_vao.get_ibo_count());
}

void GameRenderer::render_held_block(Game &game, const mat4 &view_m, const mat4 &proj_m) {
    World  &world = game.get_world();
    Player &player = world.get_player();

    double elapsed_time = game.get_elapsed_time();

    const Camera &head_cam = player.get_head_camera();
    const double exp_rot = cosf(elapsed_time * 0.8f) * 0.1f;
    const double exp_pos = sinf(elapsed_time * 1.2f) * 0.035f;

    const vec3d vec_up = head_cam.calc_direction_up();
    const vec3d vec_fw = head_cam.calc_direction();
    const vec3d vec_rt = head_cam.calc_direction_side();

    const double rotate_y = head_cam.get_rotation().x;
    const double rotate_z = head_cam.get_rotation().y;

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
    m_chunk_shader.upload_mat4("u_proj", proj_m);
    m_chunk_shader.upload_mat4("u_view", view_m);
    m_chunk_shader.upload_mat4("u_model", transform);
    m_chunk_shader.upload_int("u_load_radius", -1);
    m_chunk_shader.upload_int("u_texture_array", 0);
    m_chunk_shader.upload_int("u_skybox", 1);
    m_chunk_tex_array.bind_texture_unit(0);
    m_skybox_cubemap.bind_cubemap_unit(1);

    m_block_vao.bind_vao();
    draw_elements_triangles(m_block_vao.get_ibo_count());
}

void GameRenderer::render_target_block(Game &game, const mat4 &view_m, const mat4 &proj_m) {
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
        SimpleDraw::draw_cube_line_outline(block_position, vec3d::make(1), { 0.0f, 0.0f, 0.0f });
    }

    if(game.debug_raycast_draw) {
        /* DEBUG: Itersection point */
        const vec3d point_size_half = vec3d::make(0.5);
        const vec3d intersect_position = target.intersection;
        SimpleDraw::draw_cube_outline(intersect_position - point_size_half, point_size_half * 2.0, 1.0f / 128.0f, { 1.0f, 1.0f, 1.0f });

        /* DEBUG: Block's normal */
        const vec3d next_position = WorldPosition::from_block(target.block_p.block + target.normal).real;
        const vec3d block_position = target.block_p.real;
        const vec3  line_color = vec3::absolute(vec3::make(get_block_side_dir(target.side)));
        SimpleDraw::draw_line(block_position + vec3d::make(0.5), next_position + vec3d::make(0.5), 2.0f, line_color);
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

    Camera &camera = game.get_camera();
    World  &world  = game.get_world();
    Player &player = world.get_player();

    debug_line("Build: %s", BUILD_TYPE);
    debug_line(NULL);

    debug_line("--- Frame ---"); {
        GameTime time = game.get_time();

        debug_line("fps: %d", calc_fps(time.delta_time));
        debug_line("delta time:   %f", time.delta_time);
        debug_line("elapsed time: %f", time.elapsed_time);
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
        vec3d cam_dir = camera.calc_direction();
        vec3d cam_pos = camera.get_position();

        debug_line("position:  %+.3f, %+.3f, %+.3f", cam_pos.x, cam_pos.y, cam_pos.z);
        debug_line("direction: %+.3f, %+.3f, %+.3f", cam_dir.x, cam_dir.y, cam_dir.z);
    } debug_line(NULL);

    debug_line("--- World ---"); {
        ChunkHashTable &chunk_table = world.get_chunk_table();
        GameThreadInfo thread_info = game.get_thread_info();

        debug_line("chunk size: %d, %d, %d", CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z);
        debug_line("world seed: %u", world.get_gen_seed().seed);
        debug_line("load radius: %d", game.get_load_radius());
        debug_line("loaded chunks: %u", chunk_table.get_count());
        debug_line("chunk buckets: %u", chunk_table.get_size());
        debug_line("gen chunks threads: %d", thread_info.gen_chunks_threads_active);
        debug_line("gen meshes threads: %d", thread_info.gen_chunks_threads_active);
        debug_line("chunks to generate: %d", thread_info.queued_chunks_num);
        debug_line("meshes to build:    %d", thread_info.queued_meshes_num);
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

    /* Render held block text */ {
        m_batcher.begin();
        const vec2 text_pos = vec2{ 6.0f, -m_ui_font_big.get_descent() * m_ui_font_big.get_scale_for_pixel_height() };
        m_batcher.push_text_formatted(text_pos, m_ui_font_big, "Held block: %s", block_type_string[(int32_t)world.get_player().get_held_block()]);
        m_batcher.render(m_width, m_height, m_ui_font_big, { 1.0f, 1.0f, 1.0f }, { 4, -4 });
    }
}


#if 0

void Game::render_frame(void) {
    mat4 proj_m = m_camera.calc_proj(m_aspect);

    Player &player = m_world.get_player();

    /* Gather chunk vao triangles rendered */
    m_triangles_rendered_last_frame = 0;

    set_viewport({ 0, 0, m_width, m_height });

    m_fbo_ms.clear_all({ 0.1f, 0.1f, 0.1f, 1.0f });
    m_fbo_ms.bind_fbo();

    // Render world to multisampled framebuffer
    this->render_world(proj_m);

    this->render_skybox(proj_m);
    this->render_water(proj_m);
    this->render_held_block(proj_m, m_aspect);
    this->render_target_block(proj_m);

    /* Resolve multisampled framebuffer to normal framebuffer */
    m_fbo.clear_all({ 0.1f, 0.1f, 0.1f, 1.0f });
    m_fbo_ms.blit_color_attachment(0, m_fbo, 0);
    m_fbo_ms.blit_color_attachment(1, m_fbo, 1);
    m_fbo_ms.blit_color_attachment(2, m_fbo, 2);
    m_fbo_ms.blit_color_attachment(3, m_fbo, 3);
    m_fbo_ms.blit_depth_attachment(m_fbo);

    switch(debug_world_blit_mode) {
        case WorldBlitMode::COLOR: {
            set_render_state({
                .blend = BlendFunc::DISABLE,
                .depth = DepthFunc::DISABLE,
                .multisample = true,
            });

            int32_t is_camera_in_water = 0;

            /* Check if camera is in water */ {
                const vec3d head_position = player.get_head_camera().get_position();
                const vec3i head_block = WorldPosition::from_real(head_position).block;
                BlockType block = m_world.get_block(head_block);
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

        case WorldBlitMode::NORMALS: {
            m_fbo.blit_color_attachment(1, m_width, m_height);
        } break;

        case WorldBlitMode::DEPTH: {
            m_fbo.blit_color_attachment(2, m_width, m_height);
        } break;

        case WorldBlitMode::AMBIENT_OCCLUSION: {
            m_fbo.blit_color_attachment(3, m_width, m_height);
        } break;
    }


    bind_no_fbo();
    this->render_ui();
    this->render_crosshair();
}

void Game::render_world(const mat4 &proj_m) {
    Player &player = m_world.get_player();
    Camera &camera = m_camera;

    m_chunk_shader.use_program();
    m_chunk_shader.upload_mat4("u_proj", proj_m);
    m_chunk_shader.upload_int("u_texture_array", 0);
    m_chunk_shader.upload_int("u_load_radius", m_load_radius);
    m_chunk_tex_array.bind_texture_unit(0);

    m_chunk_shader.upload_int("u_skybox", 3);
    m_skybox_cubemap.bind_cubemap_unit(3);


    set_render_state({
        .blend = BlendFunc::DISABLE,
        .depth = DepthFunc::LESS,
        .multisample = true,
        .cull_faces = true
    });

    if(debug_chunk_wireframe_mode) {
        set_line_width(1.0f);
        set_polygon_mode(true);
    }

    double frustum[6][4];
    camera.calc_frustum_at_origin(frustum, m_aspect);

    ChunkHashTable::Iterator iter;
    while(m_world.m_chunk_table.iterate_all(iter)) {
        Chunk *chunk = iter.value;

        const VertexArray &chunk_vao = chunk->get_vao();
        if(!chunk_vao.has_been_created() || !chunk_vao.get_ibo_count()) {
            continue;
        }

        vec3d chunk_origin = camera.offset_to_relative(real_position_from_chunk(chunk->get_chunk_xz()));

        if(!is_chunk_in_frustum(frustum, chunk_origin)) {
            continue;
        }

        vec3 chunk_translate = vec3::make(m_camera.offset_to_relative(vec3d{
            .x = (double)(iter.key.x * CHUNK_SIZE_X),
            .y = chunk->get_chunk_render_offset_y(),
            .z = (double)(iter.key.y * CHUNK_SIZE_Z) 
        }));

        m_chunk_shader.upload_mat4("u_view", m_camera.calc_view_at_origin());
        m_chunk_shader.upload_mat4("u_model", mat4::translate(chunk_translate));

        chunk_vao.bind_vao();
        draw_elements_triangles(chunk_vao.get_ibo_count());

        m_triangles_rendered_last_frame += chunk_vao.get_ibo_count() / 3;
    }

    if(debug_chunk_wireframe_mode) {
        set_polygon_mode(false);
    }

    set_render_state({
        .blend = BlendFunc::DISABLE,
        .depth = DepthFunc::LESS,
        .multisample = true,
        .cull_faces = false
    });

    /* Render shape in third person mode */
    if(m_3rd_person_mode) {
        const vec3 color = { 0.7f, 0.9f, 0.6f };
        const vec3d position = player.get_position_origin();
        const vec3d size = player.get_collider_size();
        SimpleDraw::draw_cube_outline(position, size, 2.0f / 32.0f, color);
    }

    /* Render player's debug stuff */
    if(debug_player_draw) {
        player.debug_render(*this);
    }

    /* Render debug chunk borders */
    if(debug_show_chunk_borders) {
        ChunkHashTable::Iterator iter;
        while(m_world.m_chunk_table.iterate_all(iter)) {
            Chunk *chunk = iter.value;

            /* Only render chunk border if has been generated */
            if(chunk->get_chunk_state() != ChunkState::GENERATED) {
                continue;
            }

            vec3d chunk_pos = { 
                (double)(chunk->get_chunk_xz().x * CHUNK_SIZE_X),
                0.0,
                (double)(chunk->get_chunk_xz().y * CHUNK_SIZE_Z)
            };

            SimpleDraw::draw_cube_outline(chunk_pos, { CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z }, 8.0f / 16.0f, { 1.0f, 1.0f, 1.0f }, 0.15f, { 0.1f, 0.1f, 0.1f });
        }
    }
}

void Game::render_water(const mat4 &proj_m) {
    Camera &camera = m_camera;

    set_render_state({
        .blend = BlendFunc::STANDARD,
        .depth = DepthFunc::LESS,
        .multisample = true,
        .cull_faces = true,
        .disable_depth_write = true
    });

    m_water_shader.use_program();
    m_water_shader.upload_mat4("u_proj", proj_m);
    m_water_shader.upload_float("u_time_elapsed", m_time_elapsed);
    m_water_shader.upload_int("u_water_texture_array", 0);
    m_water_shader.upload_int("u_load_radius", m_load_radius);
    m_water_tex_array.bind_texture_unit(0);
    m_water_shader.upload_int("u_skybox", 1);
    m_skybox_cubemap.bind_cubemap_unit(1);

    std::vector<Chunk *> chunks;

    double frustum[6][4];
    camera.calc_frustum_at_origin(frustum, m_aspect);

    /* Go through loaded chunks and collect water to render */
    ChunkHashTable::Iterator iter;
    while(m_world.m_chunk_table.iterate_all(iter)) {
        Chunk *chunk = iter.value;

        const VertexArray &water_vao = chunk->get_water_vao();
        if(!water_vao.has_been_created() || !water_vao.get_ibo_count()) {
            continue;
        }

        vec3d chunk_origin = camera.offset_to_relative(real_position_from_chunk(chunk->get_chunk_xz()));
        if(!is_chunk_in_frustum(frustum, chunk_origin)) {
            continue;
        }

        chunks.push_back(chunk);
    }

    /* Sort water, @TODO : IDK */
    std::sort(chunks.begin(), chunks.end(), [] (Chunk *a, Chunk *b) { 
        vec2 chunk = vec2::make(a->get_world()->get_player().get_position_chunk()); // @TODO
        float dist_a = vec2::length_sq(chunk - vec2::make(a->get_chunk_xz()));
        float dist_b = vec2::length_sq(chunk - vec2::make(b->get_chunk_xz()));
        return dist_a > dist_b; 
    });

    /* Render water meshes */
    for(auto &chunk : chunks) {

        const vec2i coords = chunk->get_chunk_xz();

        vec3 chunk_translate = vec3::make(m_camera.offset_to_relative(vec3d{
            .x = (double)(coords.x * CHUNK_SIZE_X),
            .y = chunk->get_chunk_render_offset_y(),
            .z = (double)(coords.y * CHUNK_SIZE_Z) 
        }));

        m_water_shader.upload_mat4("u_view", m_camera.calc_view_at_origin());
        m_water_shader.upload_mat4("u_model", mat4::translate(chunk_translate));

        const VertexArray &water_vao = chunk->get_water_vao();
        water_vao.bind_vao();
        draw_elements_triangles(water_vao.get_ibo_count());

        m_triangles_rendered_last_frame += water_vao.get_ibo_count() / 3;
    }
}



void Game::render_single_block(BlockType type, const mat4 &model_m, const mat4 &proj_m, const mat4 &view_m) {
    /* @todo Do not do this every frame @TODO */

}



#endif

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

