#include "game.h"
#include "window.h"
#include "simple_draw.h"
#include "opengl_abs.h"

#define STRING_VIU_CPP_HELPERS
#include <string_viu.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_thread.h>

#include <cstdarg>

namespace {
    /* Move to _Game_? */
    static int32_t  g_load_radius = 8;
    static int32_t  g_deload_radius = 16;
    static float    g_third_person_distance = 10.0f;
    static bool     g_third_person_mode = false;
    static uint32_t g_triangles_rendered_last_frame = 0;

    /* Debug draw stuff */
    static bool g_debug_show_ui_info = false;
    static bool g_debug_show_chunk_borders = false;
    static bool g_debug_player_draw = false;
    static bool g_debug_chunk_wireframe_mode = false;
    static bool g_debug_raycast_draw = false;
    static WorldBlitMode g_debug_world_blit_mode = WorldBlitMode::COLOR; // WorldBlitMode::AMBIENT_OCCLUSION;

    /* For _spawn_ command */
    static vec3i g_debug_spawn_p1 = { 0, 0, 0 };
    static vec3i g_debug_spawn_p2 = { 200, 200, 200 };
}

int32_t get_load_radius(void) {
    return g_load_radius;
}

int32_t get_deload_radius(void) {
    return g_load_radius + g_deload_radius;
}

Game::Game(Window &window) : m_world(this) {
    m_width  = window.get_width();
    m_height = window.get_height();

    m_console.initialize();

    /* Initialize threads */
    m_gen_chunks_condition = SDL_CreateCondition();
    m_gen_meshes_condition = SDL_CreateCondition();
    this->start_threads(1, 1);

    /* Create VAO's, Framebuffers and stuff */
    this->initialize_render_resources();

    m_world.initialize_new_world(rand() % 400);

    this->add_console_commands();
}

Game::~Game(void) {
    this->stop_threads();
    SDL_DestroyCondition(m_gen_chunks_condition);
    SDL_DestroyCondition(m_gen_meshes_condition);
    this->delete_render_resources();
    m_console.destroy();
}

void Game::update(Window &window, const Input &input, double delta_time) {
    m_delta_time = delta_time;
    m_time_elapsed += delta_time;

    /* Maybe close game */
    if(!m_console.is_open() && input.key_pressed(Key::ESCAPE)) {
        window.set_should_close();
        return;
    }

    /* Toggle debug ui */
    if(input.key_pressed(Key::F03)) {
        BOOL_TOGGLE(g_debug_show_ui_info);
    }

    /* Update Console */ {
        /* Open console */
        if(!m_console.is_open() && input.key_pressed(Key::T)) {
            m_console.set_open_state(true, window);
        }

        /* Close console */
        if(m_console.is_open() && input.key_pressed(Key::ESCAPE)) {
            m_console.set_open_state(false, window);
        }

        /* Set relative mouse mode if needed */
        if(!m_console.is_open() && !window.is_rel_mouse_active()) {
            window.set_rel_mouse_active(true);
        } else if(m_console.is_open() && window.is_rel_mouse_active()) {
            window.set_rel_mouse_active(false);
        }

        m_console.update(*this, input, window, delta_time);
    }

    /* Toggle third person camera */
    if(input.key_pressed(Key::F05) && !m_console.is_open()) {
        BOOL_TOGGLE(g_third_person_mode);
    }

    /* Zoom in/out third person camera */
    if(g_third_person_mode && (input.key_is_down(Key::PAGE_UP) || input.key_is_down(Key::PAGE_DOWN)) && !m_console.is_open()) {
        float dir = 0.0f;
        if(input.key_is_down(Key::PAGE_UP))   { dir -= 1.0f; } 
        if(input.key_is_down(Key::PAGE_DOWN)) { dir += 1.0f; }

        g_third_person_distance += dir * 30.0f * delta_time;
        clamp_v(g_third_person_distance, 1.0f, 20.0f);
    }

    /* Update player state */
    Player &player = m_world.get_player();
    player.update(*this, input, delta_time);

    /* Set render camera */
    if(g_third_person_mode) {
        m_camera = player.get_3rd_person_camera(g_third_person_distance);
    } else {
        m_camera = player.get_head_camera();
    }

    /* Queue generation of new chunks */
    if(player.moved_chunk_last_frame()) {
        m_world.create_chunks_in_range_offload(player.get_position_chunk(), g_load_radius + 1);
    }

    /* Insert generated chunks, and queue mesh generation */
    while(m_world.m_chunks_generated.size()) {
        SDL_LockMutex(m_world.m_lock_chunk_gen);
        ChunkGenData gen_data = m_world.m_chunks_generated.back();
        m_world.m_chunks_generated.pop_back();
        SDL_UnlockMutex(m_world.m_lock_chunk_gen);

        Chunk **chunk_hash = m_world.m_chunk_table.find(gen_data.chunk_xz);
        if(!chunk_hash) {
            continue;
        }

        Chunk *chunk = *chunk_hash;
        if(chunk->get_chunk_state() == ChunkState::GENERATED) {
            /* For some reason, the generated chunk already has been generated... ignore it */
            continue;
        }

        chunk->set_chunk_blocks(gen_data.blocks);
        chunk->set_chunk_state(ChunkState::GENERATED);
        chunk->set_appear_animation();

        /* If chunk's neighbours are generated -> queue mesh generation, otherwise -> set waiting state */
        bool has_neighbours = m_world.chunk_neighbours_generated(chunk->get_chunk_xz());
        if(has_neighbours) {
            m_world.gen_chunk_mesh_offload(chunk->get_chunk_xz());
            chunk->set_mesh_state(ChunkMeshState::QUEUED);
        } else {
            chunk->set_mesh_state(ChunkMeshState::WAITING);
        }
    }

    /* Set generated meshes */
    while(m_world.m_meshes_built.size()) {
        SDL_LockMutex(m_world.m_lock_mesh_gen);
        ChunkMeshGenData *mesh_data = m_world.m_meshes_built.back();
        m_world.m_meshes_built.pop_back();
        SDL_UnlockMutex(m_world.m_lock_mesh_gen);

        Chunk *chunk = m_world.get_chunk(mesh_data->chunk_xz);

        /* if chunk == NULL -> Chunk has been deleted during the mesh generation so ignore it */
        /* Set generated mesh if the state isn't LOADED (could happen if immediatelly loading later) */
        if(chunk != NULL && chunk->get_mesh_state() != ChunkMeshState::LOADED) {
            chunk->set_mesh(mesh_data);
            chunk->set_mesh_state(ChunkMeshState::LOADED);
        }

        chunk_mesh_gen_data_free(&mesh_data);
    }

    m_world.update_loaded_chunks(delta_time);
}

int32_t Game::thread_gen_chunks_proc(void) {
    int32_t ret_val = 0;

    for(; m_threads_keep_looping ;) {
        ChunkGenData gen_data;
        bool gen_data_found = false;

        SDL_LockMutex(m_world.m_lock_chunk_gen);
        if(!m_world.m_chunks_to_generate.empty()) {
            gen_data = m_world.m_chunks_to_generate.front();
            m_world.m_chunks_to_generate.pop();
            gen_data_found = true;
        }
        SDL_UnlockMutex(m_world.m_lock_chunk_gen);

        if(gen_data_found) {
            /* Do the chunk generation */
            chunk_gen(gen_data);

            SDL_LockMutex(m_world.m_lock_chunk_gen);
            m_world.m_chunks_generated.push_back(gen_data);
            SDL_UnlockMutex(m_world.m_lock_chunk_gen);
        }

        /* If nothing to generate -> sleep thread */
        SDL_LockMutex(m_world.m_lock_chunk_gen);
        if(m_world.m_chunks_to_generate.empty()) {
            SDL_WaitCondition(m_gen_chunks_condition, m_world.m_lock_chunk_gen);
        }
        SDL_UnlockMutex(m_world.m_lock_chunk_gen);
    }

    return ret_val;
}

int32_t Game::thread_gen_meshes_proc(void) {
    int32_t ret_val = 0;

    for(; m_threads_keep_looping ;) {
        ChunkMeshGenData *mesh_data = NULL;

        SDL_LockMutex(m_world.m_lock_mesh_gen);
        if(!m_world.m_meshes_to_build.empty()) {
            mesh_data = m_world.m_meshes_to_build.front();
            m_world.m_meshes_to_build.pop();
        }
        SDL_UnlockMutex(m_world.m_lock_mesh_gen);

        if(mesh_data != NULL) {
            /* Do the mesh generation */
            chunk_mesh_gen(mesh_data);

            SDL_LockMutex(m_world.m_lock_mesh_gen);
            m_world.m_meshes_built.push_back(mesh_data);
            SDL_UnlockMutex(m_world.m_lock_mesh_gen);
        }

        /* If nothing to generate -> sleep thread */
        SDL_LockMutex(m_world.m_lock_mesh_gen);
        if(m_world.m_meshes_to_build.empty()) {
            SDL_WaitCondition(m_gen_meshes_condition, m_world.m_lock_mesh_gen);
        }
        SDL_UnlockMutex(m_world.m_lock_mesh_gen);
    }

    return ret_val;
}

static int _thread_gen_chunks_proc(void *game_ptr) {
    Game *game = static_cast<Game *>(game_ptr);
    return game->thread_gen_chunks_proc();
}

static int _thread_gen_meshes_proc(void *game_ptr) {
    Game *game = static_cast<Game *>(game_ptr);
    return game->thread_gen_meshes_proc();
}

void Game::start_threads(int32_t chunks_threads, int32_t meshes_threads) {
    ASSERT(chunks_threads <= MAX_GEN_CHUNKS_THREADS && meshes_threads <= MAX_GEN_MESHES_THREADS && "Invalid number of threads!");

    if(m_threads_started) {
        this->stop_threads();
    }

    m_threads_keep_looping  = true;
    m_gen_chunks_thread_num = chunks_threads;
    m_gen_meshes_thread_num = meshes_threads;

    for(uint32_t index = 0; index < chunks_threads; ++index) {
        char thread_name[64];
        sprintf_s(thread_name, ARRAY_COUNT(thread_name), "Chunk gen thread #%u", index);
        m_gen_chunks_threads[index] = SDL_CreateThread(_thread_gen_chunks_proc, thread_name, (void *)this);
        ASSERT(m_gen_chunks_threads[index] && "Failed to create thread!");
    }

    for(uint32_t index = 0; index < meshes_threads; ++index) {
        char thread_name[64];
        sprintf_s(thread_name, ARRAY_COUNT(thread_name), "Chunk vao thread #%u", index);
        m_gen_meshes_threads[index] = SDL_CreateThread(_thread_gen_meshes_proc, thread_name, (void *)this);
        ASSERT(m_gen_meshes_threads[index] && "Failed to create thread!");
    }

    m_threads_started = true;
}

void Game::start_threads(void) { 
    this->start_threads(m_gen_chunks_thread_num, m_gen_meshes_thread_num);
}

void Game::stop_threads(void) {
    /* Allow threads to finish */
    m_threads_keep_looping = false;
    this->wake_up_gen_chunks_threads();
    this->wake_up_gen_meshes_threads();

    for(uint32_t index = 0; index < m_gen_chunks_thread_num; ++index) {
        if(m_gen_chunks_threads[index]) {
            SDL_WaitThread(m_gen_chunks_threads[index], NULL);
            m_gen_chunks_threads[index] = NULL;
        }
    }

    for(uint32_t index = 0; index < m_gen_meshes_thread_num; ++index) {
        if(m_gen_meshes_threads[index]) {
            SDL_WaitThread(m_gen_meshes_threads[index], NULL);
            m_gen_meshes_threads[index] = NULL;
        }
    }

    m_threads_started = false;
}

void Game::render_frame(void) {
    Player &player = m_world.get_player();

    const float aspect_ratio = (float)m_width / (float)m_height;
    mat4 proj_m = m_camera.calc_proj(aspect_ratio);

    /* Gather chunk vao triangles rendered */
    g_triangles_rendered_last_frame = 0;

    set_viewport({ 0, 0, m_width, m_height });

    m_fbo_ms.clear_all({ 0.1f, 0.1f, 0.1f, 1.0f });
    m_fbo_ms.bind_fbo();

    this->render_world(proj_m);

    switch(g_debug_world_blit_mode) {
        case WorldBlitMode::COLOR: {
            set_render_state({
                .blend = BlendFunc::DISABLE,
                .depth = DepthFunc::DISABLE,
                .multisample = true,
            });

            m_fbo.clear_all({ 0.1f, 0.1f, 0.1f, 1.0f });
            m_fbo_ms.blit_color_attachment(0, m_fbo, 0);

            int32_t is_camera_in_water = 0;
            /* Check if camera is in water */ {
                BlockType block = m_world.get_block(WorldPosition::from_real(player.get_head_camera().get_position()).block);
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
            m_fbo_ms.blit_color_attachment(1, m_width, m_height);
        } break;

        case WorldBlitMode::DEPTH: {
            m_fbo_ms.blit_color_attachment(2, m_width, m_height);
        } break;

        case WorldBlitMode::AMBIENT_OCCLUSION: {
            m_fbo_ms.blit_color_attachment(3, m_width, m_height);
        } break;
    }

    /* Copy chunks' depth values to backbuffer */
    m_fbo_ms.blit_depth_attachment(m_width, m_height);

    bind_no_fbo();
    this->render_skybox(proj_m);
    this->render_water(proj_m);
    this->render_target_block(proj_m);
    this->render_held_block(proj_m, aspect_ratio);
    this->render_ui();
    this->render_crosshair();
}

void Game::render_world(const mat4 &proj_m) {
    Player &player = m_world.get_player();

    m_chunk_shader.use_program();
    m_chunk_shader.upload_mat4("u_proj", proj_m);
    m_chunk_shader.upload_int("u_texture_array", 0);
    m_chunk_tex_array.bind_texture_unit(0);

    set_render_state({
        .blend = BlendFunc::DISABLE,
        .depth = DepthFunc::LESS,
        .multisample = true,
        .cull_faces = true
    });

    if(g_debug_chunk_wireframe_mode) {
        set_line_width(1.0f);
        set_polygon_mode(true);
    }

    /* Iterate chunks and render them, @TODO : Frustum cull */
    ChunkHashTable::Iterator iter;
    while(m_world.m_chunk_table.iterate_all(iter)) {
        Chunk *chunk = iter.value;

        const VertexArray &chunk_vao = chunk->get_vao();
        if(!chunk_vao.has_been_created() || !chunk_vao.get_ibo_count()) {
            continue;
        }

        vec3 chunk_translate = {
            .x = 0.0f,
            .y = chunk->get_chunk_render_offset_y(),
            .z = 0.0f
        };

        vec3 camera_offset = { 
            .x = (float)(iter.key.x * CHUNK_SIZE_X), 
            .y = 0.0f, 
            .z = (float)(iter.key.y * CHUNK_SIZE_Z) 
        };

        m_chunk_shader.upload_mat4("u_view", m_camera.calc_view(camera_offset));
        m_chunk_shader.upload_mat4("u_model", mat4::translate(chunk_translate));

        chunk_vao.bind_vao();
        draw_elements_triangles(chunk_vao.get_ibo_count());

        g_triangles_rendered_last_frame += chunk_vao.get_ibo_count() / 3;
    }

    if(g_debug_chunk_wireframe_mode) {
        set_polygon_mode(false);
    }

    set_render_state({
        .blend = BlendFunc::DISABLE,
        .depth = DepthFunc::LESS,
        .multisample = true,
        .cull_faces = false
    });

    /* Render shape in third person mode */
    if(g_third_person_mode) {
        const vec3 color = { 0.7f, 0.9f, 0.6f };
        SimpleDraw::draw_cube_outline(player.get_position_origin(), player.get_collider_size(), 2.0f / 32.0f, color);
    }

    /* Render player's debug stuff */
    if(g_debug_player_draw) {
        player.debug_render(*this);
    }

    /* Render debug chunk borders */
    if(g_debug_show_chunk_borders) {
        ChunkHashTable::Iterator iter;
        while(m_world.m_chunk_table.iterate_all(iter)) {
            Chunk *chunk = iter.value;

            /* Only render chunk border if has been generated */
            if(chunk->get_chunk_state() != ChunkState::GENERATED) {
                continue;
            }

            vec3 chunk_pos = { 
                float(chunk->get_chunk_xz().x * CHUNK_SIZE_X),
                0.0f,
                float(chunk->get_chunk_xz().y * CHUNK_SIZE_Z)
            };

            SimpleDraw::draw_cube_outline(chunk_pos, { CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z }, 8.0f / 16.0f, { 1.0f, 1.0f, 1.0f }, 0.15f, { 0.1f, 0.1f, 0.1f });
        }
    }
}

void Game::render_water(const mat4 &proj_m) {
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
    m_water_tex_array.bind_texture_unit(0);
    m_water_shader.upload_int("u_skybox", 1);
    m_skybox_cubemap.bind_cubemap_unit(1);

    std::vector<Chunk *> chunks;

    /* Go through loaded chunks and collect water to render */
    ChunkHashTable::Iterator iter;
    while(m_world.m_chunk_table.iterate_all(iter)) {
        Chunk *chunk = iter.value;

        const VertexArray &water_vao = chunk->get_water_vao();
        if(!water_vao.has_been_created() || !water_vao.get_ibo_count()) {
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

        vec3 chunk_translate = {
            .x = 0.0f,
            .y = chunk->get_chunk_render_offset_y(),
            .z = 0.0f
        };

        vec3 camera_offset = { 
            .x = (float)(chunk->get_chunk_xz().x * CHUNK_SIZE_X), 
            .y = 0.0f, 
            .z = (float)(chunk->get_chunk_xz().y * CHUNK_SIZE_Z) 
        };

        m_water_shader.upload_mat4("u_view", m_camera.calc_view(camera_offset));
        m_water_shader.upload_mat4("u_model", mat4::translate(chunk_translate));

        const VertexArray &water_vao = chunk->get_water_vao();
        water_vao.bind_vao();
        draw_elements_triangles(water_vao.get_ibo_count());

        g_triangles_rendered_last_frame += water_vao.get_ibo_count() / 3;
    }
}

void Game::render_target_block(const mat4 &proj_m) {
    RaycastBlockResult target = m_world.get_player().get_targeted_block();
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
    SimpleDraw::draw_cube_line_outline(target.block_p.real, vec3::make(1), { 0.0f, 0.0f, 0.0f });

    if(g_debug_raycast_draw) {
        /* DEBUG: Itersection point */
        const float point_size = 0.1f;
        SimpleDraw::draw_cube_outline(target.intersection - vec3::make(point_size * 0.5f), vec3::make(point_size), 1.0f / 128.0f, { 1.0f, 1.0f, 1.0f });

        /* DEBUG: Block's normal */
        const WorldPosition next = WorldPosition::from_block(target.block_p.block + target.normal);
        SimpleDraw::draw_line(target.block_p.real + vec3::make(0.5f), next.real + vec3::make(0.5f), 2.0f, vec3::absolute(vec3::make(get_block_side_dir(target.side))));
    }
}

void Game::render_skybox(const mat4 &proj_m) {
    set_render_state({
        .blend = BlendFunc::DISABLE,
        .depth = DepthFunc::LESS_OR_EQUAL,
        .multisample = true
    });

    m_skybox_shader.use_program();
    m_skybox_shader.upload_mat4("u_proj", proj_m);
    m_skybox_shader.upload_mat4("u_view", m_camera.calc_view());
    m_skybox_vao.bind_vao();
    m_skybox_cubemap.bind_cubemap();
    draw_elements_triangles(m_skybox_vao.get_ibo_count());
}

void Game::render_held_block(const mat4 &proj_m, float aspect_ratio) {
    const Camera &head_cam = m_world.get_player().get_head_camera();
    const float exp_rot = cosf(m_time_elapsed * 0.8f) * 0.1f;
    const float exp_pos = sinf(m_time_elapsed * 1.2f) * 0.035f;

    const vec3 vec_up = head_cam.calc_direction_up();
    const vec3 vec_fw = head_cam.calc_direction();
    const vec3 vec_rt = head_cam.calc_direction_side();

    const float rotate_y = head_cam.get_rotation().x;
    const float rotate_z = head_cam.get_rotation().y;

    const vec3 size = vec3::make(0.16f);
    const vec3 position = vec_fw * 0.4f + vec_rt * 0.155f * aspect_ratio - vec_up * (0.28f + exp_pos);

    mat4 transform;
    transform = mat4::scale(size);        
    transform *= mat4::rotate_z(-rotate_z);
    transform *= mat4::rotate_y(rotate_y);
    transform *= mat4::translate(position);

    Camera camera = this->get_camera();
    camera.set_position({ 0.0f, 0.0f, 0.0f });
    mat4 view_m = camera.calc_view();

    set_render_state({
        .blend = BlendFunc::STANDARD,
        .depth = DepthFunc::ALWAYS,
        .multisample = true,
        .cull_faces = true
    });

    BlockType held_block = m_world.get_player().get_held_block();
    this->render_single_block(held_block, transform, proj_m, view_m);
}

void Game::render_single_block(BlockType type, const mat4 &model_m, const mat4 &proj_m, const mat4 &view_m) {
    /* @todo Do not do this every frame @TODO */

    ChunkMeshData mesh_data;
    chunk_mesh_gen_single_block(mesh_data, type);

    m_block_vao.set_vbo_data(mesh_data.vertices.data(), mesh_data.vertices.size() * sizeof(ChunkVaoVertexPacked));
    m_block_vao.set_ibo_data(mesh_data.indices.data(), mesh_data.indices.size());
    m_block_vao.apply_vao_attributes();

    m_chunk_shader.use_program();
    m_chunk_shader.upload_mat4("u_proj", proj_m);
    m_chunk_shader.upload_mat4("u_view", view_m);
    m_chunk_shader.upload_mat4("u_model", model_m);
    m_chunk_shader.upload_int("u_texture_array", 0);
    m_chunk_tex_array.bind_texture_unit(0);

    m_block_vao.bind_vao();
    draw_elements_triangles(m_block_vao.get_ibo_count());
}

void Game::render_crosshair(void) {
    set_render_state({
        .blend = BlendFunc::STANDARD,
        .depth = DepthFunc::DISABLE,
        .multisample = true,
        .cull_faces = false
    });

    const int32_t crosshair_size  = 18;
    const int32_t crosshair_width = 2;

    const vec2i translate = {
        m_width / 2 - crosshair_size / 2,
        m_height / 2 - crosshair_size / 2
    };

    mat4 proj_m = mat4::orthographic(0.0f, 0.0f, m_width, m_height, -1.0f, 1.0f);
    mat4 model_m = mat4::scale(crosshair_size, crosshair_size, 1.0f) * mat4::translate(translate.x, translate.y, 0.0f);

    m_crosshair_shader.use_program();
    m_crosshair_shader.upload_mat4("u_proj", proj_m);
    m_crosshair_shader.upload_mat4("u_model", model_m);
    m_crosshair_shader.upload_float("u_perc", (float)crosshair_width / (float)crosshair_size);

    m_crosshair_vao.bind_vao();
    draw_elements_triangles(m_crosshair_vao.get_ibo_count());
}

void Game::render_ui(void) {
    /* Render held block text */ {
        TextBatcher batcher;
        batcher.begin();
        const vec2 text_pos = vec2{ 6.0f, -m_ui_font_big.get_descent() * m_ui_font_big.get_scale_for_pixel_height() };
        batcher.push_text_formatted(text_pos, m_ui_font_big, "Held block: %s", block_type_string[(int32_t)m_world.get_player().get_held_block()]);
        batcher.render(m_width, m_height, m_ui_font_big, { 1.0f, 1.0f, 1.0f }, { 4, -4 });
    }

    if(g_debug_show_ui_info) {
        this->render_ui_debug_info();
    }

    m_console.render(m_width, m_height);
}

static int32_t calc_fps(double delta_time);

void Game::render_ui_debug_info(void) {
    Font &font = m_ui_font_smooth;

    const float spacing = 2.0f; // 8.0f;
    const vec2  padding = { 4.0f, 4.0f };
    const float baseline = m_height - padding.y - font.get_ascent() * font.get_scale_for_pixel_height();

    float cursor_x = padding.x;
    float cursor_y = baseline;

    TextBatcher batcher;
    batcher.begin();

    auto debug_line = [&] (const char *format, ...) {
        if(format != NULL) {
            va_list args;
            va_start(args, format);
            batcher.push_text_va_args({ cursor_x, cursor_y }, font, format, args);
            va_end(args);
        }
        cursor_y -= font.get_height() + spacing;
    };

    Player &player = m_world.get_player();

    WorldPosition player_p = WorldPosition::from_real(player.get_position());
    Camera cam = player.get_head_camera();
    vec3 cam_dir = cam.calc_direction();

    RaycastBlockResult target_block = player.get_targeted_block();
    BlockType targeted_block = m_world.get_block(target_block.block_p.block);

    /* Push debug text here */ {
        debug_line("Build: %s", BUILD_TYPE);
        debug_line(NULL);

        debug_line("--- Frame ---");
        debug_line("fps: %d", calc_fps(m_delta_time));
        debug_line("delta time:   %f", m_delta_time);
        debug_line("elapsed time: %f", m_time_elapsed);
        debug_line(NULL);

        debug_line("--- Render ---");
        debug_line("triangles rendered: %u (%.3fmln)", g_triangles_rendered_last_frame, (double)g_triangles_rendered_last_frame / 1000000.0);
        debug_line(NULL);

        debug_line("--- Player ---");
        debug_line("real:      %+.2f, %+.2f, %+.2f", player_p.real.x, player_p.real.y, player_p.real.z);
        debug_line("block:     %+02d, %+02d, %+02d", player_p.block.x, player_p.block.y, player_p.block.z);
        debug_line("block rel: %+02d, %+02d, %+02d", player_p.block_rel.x, player_p.block_rel.y, player_p.block_rel.z);
        debug_line("chunk:     %+02d, %+02d", player_p.chunk.x, player_p.chunk.y);
        debug_line("orientation %+.3f, %+.3f, %+.3f", cam_dir.x, cam_dir.y, cam_dir.z);
        debug_line(NULL);

        debug_line("--- World ---");
        debug_line("chunk size: %d, %d, %d", CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z);
        debug_line("world seed: %u", m_world.get_gen_seed().seed);
        debug_line("load radius: %d", g_load_radius);
        debug_line("loaded chunks: %u", m_world.m_chunk_table.get_count());
        debug_line("chunk buckets: %u", m_world.m_chunk_table.get_size());
        debug_line("gen chunks threads: %d", m_gen_chunks_thread_num);
        debug_line("gen meshes threads: %d", m_gen_meshes_thread_num);
        debug_line("chunks to generate: %d", m_world.m_chunks_to_generate.size());
        debug_line("chunks generated:   %d", m_world.m_chunks_generated.size());
        debug_line("meshes to build:    %d", m_world.m_meshes_to_build.size());
        debug_line("meshes built:       %d", m_world.m_meshes_built.size());
        debug_line(NULL);

        if(targeted_block != BlockType::_INVALID) {
            debug_line("--- Target Block ---");
            debug_line("block: %s", block_type_string[(uint32_t)targeted_block]);
            debug_line("block abs: %+02d, %+02d, %+02d", target_block.block_p.block.x, target_block.block_p.block.y, target_block.block_p.block.z);
            debug_line(NULL);
        }
    }

    batcher.render(m_width, m_height, font, { 1.0f, 1.0f, 1.0f }, { 4, -4 });
}

void Game::hotload_shaders(void) {
    m_chunk_shader.hotload();
    m_water_shader.hotload();
    m_skybox_shader.hotload();
    m_post_process_shader.hotload();
    m_crosshair_shader.hotload();
}

void Game::resize(int32_t width, int32_t height) {
    m_width = width;
    m_height = height;
    m_fbo_ms.resize_fbo(width, height);
    m_fbo.resize_fbo(width, height);
}

void Game::wake_up_gen_chunks_threads(void) {
    if(m_gen_chunks_thread_num) {
        SDL_BroadcastCondition(m_gen_chunks_condition);
    }
}

void Game::wake_up_gen_meshes_threads(void) {
    if(m_gen_meshes_thread_num) {
        SDL_BroadcastCondition(m_gen_meshes_condition);
    }
}

World &Game::get_world(void) {
    return m_world;
}

Camera &Game::get_camera(void) {
    return m_camera;
}

Console &Game::get_console(void) {
    return m_console;
}

CONSOLE_COMMAND_PROC(command_set_threads) {
    if(args.size() != 2) {
        console.add_to_history("Invalid number of arguments, 2 required.");
        return;
    }

    int32_t chunks_threads = std::stoi(s_viu_to_std_string(args[0]));
    int32_t meshes_threads = std::stoi(s_viu_to_std_string(args[1]));

    if(chunks_threads < 0 || meshes_threads < 0 || chunks_threads > MAX_GEN_CHUNKS_THREADS || meshes_threads > MAX_GEN_MESHES_THREADS) {
        console.add_to_history("Invalid number of threads specified.");
        return;
    }

    /* Stop active threads then create new threads */
    game.stop_threads();
    game.start_threads(chunks_threads, meshes_threads);
}

CONSOLE_COMMAND_PROC(command_reload_meshes) {
    if(args.size() != 0) {
        console.add_to_history("The command does not take arguments.");
        return;
    }

    ChunkHashTable &table = game.get_world().get_chunk_table();
    ChunkHashTable::Iterator iter;
    while(table.iterate_all(iter)) {
        iter.value->set_wait_for_mesh_reload();
    }
}

CONSOLE_COMMAND_PROC(command_reset_world) {
    int32_t seed;
    if(args.size() == 0) {
        seed = game.get_world().get_gen_seed().seed;
    } else if(args.size() == 1) {
        seed = std::stoi(s_viu_to_std_string(args[0]));
    } else {
        console.add_to_history("Invalid number of arguments, 0 or 1 required.");
        return;
    }

    game.stop_threads();
    game.get_world().initialize_new_world(seed);
    game.start_threads();
}

CONSOLE_COMMAND_PROC(command_set_load_radius) {
    if(args.size() != 1) {
        console.add_to_history("Invalid number of arguments, 1 required.");
        return;
    }

    int32_t radius = std::stoi(s_viu_to_std_string(args[0]));
    if(radius <= 0) {
        console.add_to_history("Invalid load radius number.");
        return;
    }

    g_load_radius = radius;
}

CONSOLE_COMMAND_PROC(command_set_blit_mode) {
    if(args.size() != 1) {
        console.add_to_history("Invalid number of arguments, 1 required.");
        return;
    }

    if(args[0] == "color") {
        g_debug_world_blit_mode = WorldBlitMode::COLOR;
    } else if(args[0] == "normals") {
        g_debug_world_blit_mode = WorldBlitMode::NORMALS;
    } else if(args[0] == "depth") {
        g_debug_world_blit_mode = WorldBlitMode::DEPTH;
    } else if(args[0] == "AO") {
        g_debug_world_blit_mode = WorldBlitMode::AMBIENT_OCCLUSION;
    } else {
        console.add_to_history("Invalid blit mode argument.");
    }
}

CONSOLE_COMMAND_PROC(command_toggle) {
    if(args.size() != 1) {
        console.add_to_history("Invalid number of arguments, 1 required.");
        return;
    }

    if(args[0] == "player") {
        BOOL_TOGGLE(g_debug_player_draw);
    } else if(args[0] == "chunks") {
        BOOL_TOGGLE(g_debug_show_chunk_borders);
    } else if(args[0] == "wireframe") {
        BOOL_TOGGLE(g_debug_chunk_wireframe_mode);
    } else if(args[0] == "raycast") {
        BOOL_TOGGLE(g_debug_raycast_draw);
    } else {
        console.add_to_history("Invalid toggle argument.");
    }
}

CONSOLE_COMMAND_PROC(command_set_fov) {
    if(args.size() != 1) {
        console.add_to_history("Invalid number of arguments, 1 required.");
        return;
    }

    float fov = std::stof(s_viu_to_std_string(args[0]));
    if(fov >= 1.0f) {
        console.add_to_history("Invalid FOV value.");
    }

    game.get_world().get_player().get_head_camera().set_fov(DEG_TO_RAD(fov));
}

CONSOLE_COMMAND_PROC(command_spawn_stuff) {
    if(args.size()) {
        console.add_to_history("Command does not take arguments.");
        return;
    }

    World &world = game.get_world();

    ChunkHashTable::Iterator iter = {};
    while(world.get_chunk_table().iterate_all(iter)) {
        for(int32_t x = 0; x < CHUNK_SIZE_X; ++x) {
            for(int32_t y = 0; y < CHUNK_SIZE_Y; ++y) {
                for(int32_t z = 0; z < CHUNK_SIZE_Z; ++z) {
                    const vec3i block_rel = { x, y, z };
                    BlockType block = iter.value->get_block(block_rel);
                    if(rand() % 100 == 0) {
                        BlockType type = (BlockType)(rand() % ((int32_t)BlockType::_COUNT - 1));
                        iter.value->set_block(block_rel, type);
                    }
                }
            }
        }

        iter.value->set_wait_for_mesh_reload();
    }
}

CONSOLE_COMMAND_PROC(command_spawn_set_point_1) {
    if(args.size() != 3) {
        console.add_to_history("Invalid number of arguments, 3 required.");
        return;
    }

    int32_t x = std::stoi(s_viu_to_std_string(args[0]));
    int32_t y = std::stoi(s_viu_to_std_string(args[1]));
    int32_t z = std::stoi(s_viu_to_std_string(args[2]));
    g_debug_spawn_p1 = vec3i{ x, y, z };
}

CONSOLE_COMMAND_PROC(command_spawn_set_point_2) {
    if(args.size() != 3) {
        console.add_to_history("Invalid number of arguments, 3 required.");
        return;
    }

    int32_t x = std::stoi(s_viu_to_std_string(args[0]));
    int32_t y = std::stoi(s_viu_to_std_string(args[1]));
    int32_t z = std::stoi(s_viu_to_std_string(args[2]));
    g_debug_spawn_p2 = vec3i{ x, y, z };
}

CONSOLE_COMMAND_PROC(command_spawn) {
    if(args.size() != 1) {
        console.add_to_history("Invalid number of arguments, 1 required.");
        return;
    }

    BlockType type = (BlockType)std::stoi(s_viu_to_std_string(args[0]));
    if(!((int32_t)type >= 0 && (int32_t)type < (int32_t)BlockType::_COUNT)) {
        console.add_to_history("Invalid block type id.");
        return;
     }

    vec3i min = {
        MIN(g_debug_spawn_p1.x, g_debug_spawn_p2.x),
        MIN(g_debug_spawn_p1.y, g_debug_spawn_p2.y),
        MIN(g_debug_spawn_p1.z, g_debug_spawn_p2.z),
    };

    vec3i max = {
        MAX(g_debug_spawn_p1.x, g_debug_spawn_p2.x),
        MAX(g_debug_spawn_p1.y, g_debug_spawn_p2.y),
        MAX(g_debug_spawn_p1.z, g_debug_spawn_p2.z),
    };

    for(int32_t x = min.x; x <= max.x; x++) {
        for(int32_t y = min.y; y <= max.y; y++) {
            for(int32_t z = min.z; z <= max.z; z++) {
                WorldPosition p = WorldPosition::from_block({ x, y, z });
                if(Chunk *chunk = game.get_world().get_chunk_create(p.chunk)) {
                    chunk->set_block(p.block_rel, type);
                    chunk->set_wait_for_mesh_reload();
                }
            }
        }
    }
}

CONSOLE_COMMAND_PROC(command_set_pos) {
    if(args.size() != 3) {
        console.add_to_history("Invalid number of arguments, 3 required.");
        return;
    }

    float x = std::stof(s_viu_to_std_string(args[0]));
    float y = std::stof(s_viu_to_std_string(args[1]));
    float z = std::stof(s_viu_to_std_string(args[2]));

    game.get_world().get_player().set_position({ x, y, z });
}

CONSOLE_COMMAND_PROC(command_fly) {
    if(args.size() != 0) {
        console.add_to_history("The command does not take arguments.");
        return;
    }

    Player &player = game.get_world().get_player();
    if(player.get_movement_mode() == PlayerMovementMode::NORMAL) {
        player.set_movement_mode(PlayerMovementMode::FLYING);
    } else {
        player.set_movement_mode(PlayerMovementMode::NORMAL);
    }
}

void Game::add_console_commands(void) {
    m_console.set_command("set_threads",     command_set_threads);
    m_console.set_command("reload_meshes",   command_reload_meshes);
    m_console.set_command("reset_world",     command_reset_world);
    m_console.set_command("set_load_radius", command_set_load_radius);
    m_console.set_command("set_blit_mode",   command_set_blit_mode);
    m_console.set_command("toggle",          command_toggle);
    m_console.set_command("set_fov",         command_set_fov);
    m_console.set_command("spawn_stuff",     command_spawn_stuff);
    m_console.set_command("spawn_set_p1",    command_spawn_set_point_1);
    m_console.set_command("spawn_set_p2",    command_spawn_set_point_2);
    m_console.set_command("spawn",           command_spawn);
    m_console.set_command("set_pos",         command_set_pos);
    m_console.set_command("fly",             command_fly);
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

void Game::initialize_render_resources(void) {
    /* Initialize framebuffers */ {
        FboConfig fbo_ms_config;
        fbo_ms_config.ms_samples = 4;
        fbo_config_push_color(fbo_ms_config, { .target = FboAttachmentTarget::TEXTURE_MULTISAMPLE, .format = TextureDataFormat::RGBA });
        fbo_config_push_color(fbo_ms_config, { .target = FboAttachmentTarget::TEXTURE_MULTISAMPLE, .format = TextureDataFormat::RGBA });
        fbo_config_push_color(fbo_ms_config, { .target = FboAttachmentTarget::TEXTURE_MULTISAMPLE, .format = TextureDataFormat::RGBA });
        fbo_config_push_color(fbo_ms_config, { .target = FboAttachmentTarget::TEXTURE_MULTISAMPLE, .format = TextureDataFormat::RGBA });
        fbo_config_set_depth(fbo_ms_config,  { .target = FboAttachmentTarget::TEXTURE_MULTISAMPLE, .format = TextureDataFormat::DEPTH24_STENCIL8 });
        m_fbo_ms.create_fbo(m_width, m_height, fbo_ms_config);

        FboConfig fbo_config;
        fbo_config_push_color(fbo_config, { .target = FboAttachmentTarget::TEXTURE, .format = TextureDataFormat::RGBA });
        m_fbo.create_fbo(m_width, m_height, fbo_config);
    }

    /* Load resources */ {
        m_ui_font.load_from_file("data//MinecraftRegular-Bmg3.otf", 20);
        m_ui_font_big.load_from_file("data//MinecraftRegular-Bmg3.otf", 40);
        m_ui_font_smooth.load_from_file("data//CascadiaMono.ttf", 16);

        m_chunk_shader.set_filepath_and_load("source//shaders//chunk.glsl");
        m_chunk_tex_array.load_empty_reserve(16, 16, 64, TextureDataFormat::RGBA);

        m_water_shader.set_filepath_and_load("source//shaders//water.glsl");
        m_water_tex_array.load_empty_reserve(16, 16, 32, TextureDataFormat::RGBA);

        init_block_texture_array(m_chunk_tex_array, "data//atlas.png");
        init_water_texture_array(m_water_tex_array, "data//water.png");
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

void Game::delete_render_resources(void) {
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
