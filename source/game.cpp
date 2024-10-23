#include "game.h"

#include "console.h"
#include "debug_ui.h"
#include "simple_draw.h"

#include <glew.h>

namespace {
    static int32_t g_load_radius = 16;
    static float   g_third_person_distance = 10.0f;
    static bool    g_third_person_mode = false;

    static uint32_t g_triangles_rendered_last_frame = 0;
    static bool g_debug_show_chunk_borders   = false;
    static bool g_debug_show_player_collider = false;
    static bool g_debug_chunk_wireframe_mode = false;
}

constexpr vec3 init_player_pos = { 
    .x = CHUNK_SIZE_X * 0.5f,
    .y = 152.0f,
    .z = CHUNK_SIZE_Z * 0.5f
};

Game::Game(Window &window) : m_world(this) {
    int32_t width = window.get_width();
    int32_t height = window.get_height();
    m_fbo_world.create_fbo(width, height);
    m_fbo_ui.create_fbo(width, height);

    m_block_shader.set_filepath_and_load("C://dev//emce//source//shaders//block.glsl");
    m_block_atlas.load_from_file("C://dev//emce//data//atlas.png", true);
    m_block_atlas.set_filter_min(TextureFilter::NEAREST);
    m_block_atlas.set_filter_mag(TextureFilter::NEAREST);

    /* init skybox */ {
        const std::string folder = "skybox_blue";
        const std::string extension = ".jpg";
        const std::string filepaths[6] = {
            "C://dev//emce//data//" + folder + "//right" + extension,
            "C://dev//emce//data//" + folder + "//left" + extension,
            "C://dev//emce//data//" + folder + "//up" + extension,
            "C://dev//emce//data//" + folder + "//down" + extension,
            "C://dev//emce//data//" + folder + "//front" + extension,
            "C://dev//emce//data//" + folder + "//back" + extension,
        };

        m_skybox_cubemap.load_from_file(filepaths);
        m_skybox_cubemap.set_filter_min(TextureFilter::NEAREST);
        m_skybox_cubemap.set_filter_mag(TextureFilter::NEAREST);

        m_skybox_shader.set_filepath_and_load("C://dev//emce//source//shaders//skybox.glsl");

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

    m_player.set_position(init_player_pos);
    m_camera.set_position(m_player.get_position());
    m_camera.set_rotation({ DEG_TO_RAD(90.0f), DEG_TO_RAD(-45.0f) });

    m_world.initialize_world(14);

    this->add_console_commands();
}

Game::~Game(void) {
    m_fbo_world.delete_fbo();
    m_fbo_ui.delete_fbo();

    m_block_shader.delete_shader();
    m_block_atlas.delete_texture_if_exists();

    m_skybox_vao.delete_vao();
    m_skybox_shader.delete_shader();
    m_skybox_cubemap.delete_cubemap();
}

void Game::update(Window &window, const Input &input, double delta_time) {
    bool should_capture_mouse = !Console::is_open();
    if(should_capture_mouse && !window.is_rel_mouse_active()) {
        window.set_rel_mouse_active(true);
    } else if(!should_capture_mouse && window.is_rel_mouse_active()) {
        window.set_rel_mouse_active(false);
    }

    /* Gen new chunks */ {
        WorldPosition camera_position = WorldPosition::from_real(m_camera.get_position());
        for(int32_t i = 0; i < g_load_radius; ++i) {
            for(int32_t load_x = -i; load_x <= i; ++load_x) {
                vec2i load_xz = camera_position.chunk + vec2i{ load_x, i };
                m_world.gen_chunk(load_xz);
                load_xz = camera_position.chunk + vec2i{ load_x, -i };
                m_world.gen_chunk(load_xz);
            }
            for(int32_t load_z = -i + 1; load_z <= (i - 1); ++load_z) {
                vec2i load_xz = camera_position.chunk + vec2i{ i, load_z };
                m_world.gen_chunk(load_xz);
                load_xz = camera_position.chunk + vec2i{ -i, load_z };
                m_world.gen_chunk(load_xz);
            }
        }
    }

    if(input.key_pressed(Key::F05) && !Console::is_open()) {
        BOOL_TOGGLE(g_third_person_mode);
    }

    if(g_third_person_mode && input.scroll_move() && !Console::is_open()) {
        g_third_person_distance -= input.scroll_move();
        clamp_v(g_third_person_distance, 1.0f, 20.0f);
    }

    const vec2i last_player_chunk = WorldPosition::from_real(m_player.get_position()).chunk;

    m_player.update(*this, input, delta_time);
    m_camera = m_player.get_head_camera();

    if(g_third_person_mode) {
        vec3 dir = m_camera.calc_direction();
        m_camera.set_position(m_camera.get_position() - dir * g_third_person_distance);
    }

    if(last_player_chunk != WorldPosition::from_real(m_player.get_position()).chunk) {
        m_world.m_should_sort_load_queue = true;
    }

    /* Upload generated chunk mesh data */
    m_world.process_gen_queue();

    this->push_debug_ui();
}

void Game::render_world(void) {
    const float aspect = (float)m_fbo_world.get_width() / (float)m_fbo_world.get_height();

    m_fbo_world.clear_fbo({ 0.1f, 0.1f, 0.1f, 1.0f }, CLEAR_COLOR | CLEAR_DEPTH | CLEAR_STENCIL);
    m_fbo_world.bind_fbo();
    m_fbo_world.set_viewport();
    
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    GL_CHECK(glDepthFunc(GL_LESS));

    /* Render Chunks */ {
        g_triangles_rendered_last_frame = 0;

        m_block_shader.use_program();
        m_block_shader.upload_mat4("u_proj", m_camera.calc_proj(aspect).e);
        m_block_shader.upload_mat4("u_view", m_camera.calc_view().e);
        m_block_atlas.bind_texture_unit(0);

        if(g_debug_chunk_wireframe_mode ) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }

        ChunkHashTable::Iterator iter;
        while(m_world.m_chunk_table.iterate_all(iter)) {
            Chunk *chunk = *iter.value;

            const VertexArray &chunk_vao = chunk->get_vao();
            if(!chunk_vao.has_been_created()) {
                continue;
            }

            const int32_t x = iter.key.x;
            const int32_t z = iter.key.y;
            m_block_shader.upload_mat4("u_model", mat4::translate(vec3{ float(x * CHUNK_SIZE_X), 0.0f, float(z * CHUNK_SIZE_Z) }).e);

            chunk_vao.bind_vao();
            GL_CHECK(glDrawElements(GL_TRIANGLES, chunk_vao.get_ibo_count(), GL_UNSIGNED_INT, 0));

            g_triangles_rendered_last_frame += chunk_vao.get_ibo_count() / 3;
        }

        if(g_debug_chunk_wireframe_mode) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }

    /* Render collider in third person mode */
    if(g_third_person_mode && !g_debug_show_player_collider) {
        const Color color = { 0.7f, 0.9f, 0.6f, 1.0f };
        SimpleDraw::draw_cube_outline(m_player.get_position(), m_player.get_size(), 2.0f / 32.0f, color);
    }

    if(g_debug_show_player_collider) {
        /* Player collider */
        const Color collider_color = { 0.9f, 0.9f, 0.6f, 1.0f };
        SimpleDraw::draw_cube_outline(m_player.get_position(), m_player.get_size(), 1.0f / 32.0f, collider_color);

        /* Ground check collider */
        const Color ground_collider_color = m_player.is_grounded() ? Color{ 0.0f, 1.0f, 0.0f, 1.0f } : Color{ 1.0f, 0.0f, 0.0f, 1.0f };
        vec3 ground_collider_pos;
        vec3 ground_collider_size;
        m_player.get_ground_collider_info(ground_collider_pos, ground_collider_size);
        SimpleDraw::draw_cube_outline(ground_collider_pos, ground_collider_size, 1.0f / 32.0f, ground_collider_color);

        const vec3 velocity = m_player.get_velocity();
        SimpleDraw::draw_line(m_player.get_position_center(), m_player.get_position_center() + vec3{ velocity.x, 0.0f, velocity.z }, 2.0f, Color{ 1.0f, 0.0f, 1.0f, 1.0f });
        SimpleDraw::draw_line(m_player.get_position_center(), m_player.get_position_center() + vec3{ 0.0f, velocity.y, 0.0f }, 2.0f, Color{ 0.0f, 1.0f, 0.0f, 1.0f });

        const float _xyz_len = 4.0f;
        const vec3  _xyz_off = { 5.0f, 0.0f, 0.05f };
        const vec3  _xyz_base = _xyz_off + m_player.get_position_center();
        SimpleDraw::draw_line(_xyz_base, _xyz_base + vec3{ _xyz_len, 0.0f, 0.0f }, 2.0f, Color{ 1.0f, 0.0f, 0.0f, 1.0f });
        SimpleDraw::draw_line(_xyz_base, _xyz_base + vec3{ 0.0f, _xyz_len, 0.0f }, 2.0f, Color{ 0.0f, 1.0f, 0.0f, 1.0f });
        SimpleDraw::draw_line(_xyz_base, _xyz_base + vec3{ 0.0f, 0.0f, _xyz_len }, 2.0f, Color{ 0.0f, 0.0f, 1.0f, 1.0f });

        m_player.debug_render(*this);
    }

    if(g_debug_show_chunk_borders) {
        ChunkHashTable::Iterator iter;
        while(m_world.m_chunk_table.iterate_all(iter)) {
            Chunk *chunk = *iter.value;
            vec3 chunk_pos = { 
                float(chunk->get_coords().x * CHUNK_SIZE_X),
                0.0f,
                float(chunk->get_coords().y * CHUNK_SIZE_Z)
            };
            SimpleDraw::draw_cube_outline(chunk_pos, { CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z }, 8.0f / 16.0f, { 1.0f, 1.0f, 1.0f, 1.0f });
        }
    }

    /* Render targeted block outline */ {
        RaycastBlockResult target = m_player.get_targeted_block();
        if(target.found) {
            glDisable(GL_DEPTH_TEST);
            /* The targeted block */
            SimpleDraw::draw_cube_outline(target.block_p.real, vec3::make(1), 1.0f / 32.0f, { 1.0f, 1.0f, 1.0f, 1.0f });
            glEnable(GL_DEPTH_TEST);

            /* Itersection point */
            const float point_size = 0.1f;
            SimpleDraw::draw_cube_outline(target.intersection - vec3::make(point_size * 0.5f), vec3::make(point_size), 1.0f / 96.0f, { 1.0f, 1.0f, 1.0f, 1.0f });

            /* Normal block */
            const WorldPosition next = WorldPosition::from_block(target.block_p.block + vec3i::make(target.normal));
            // SimpleDraw::draw_cube_outline(next.real, vec3::make(1), 1.0f / 32.0f, { 1.0f, 1.0f, 1.0f, 1.0f });
            SimpleDraw::draw_line(target.block_p.real + vec3::make(0.5f), next.real + vec3::make(0.5f), 4.0f, { 1.0f, 1.0f, 1.0f, 1.0f });
        }
        glEnable(GL_DEPTH_TEST);
    }

    /* Render skybox */ {
        glDepthFunc(GL_LEQUAL);
        m_skybox_shader.use_program();
        m_skybox_shader.upload_mat4("u_proj", this->get_camera().calc_proj(aspect).e);
        m_skybox_shader.upload_mat4("u_view", this->get_camera().calc_view().e);
        m_skybox_vao.bind_vao();
        m_skybox_cubemap.bind_cubemap();
        GL_CHECK(glDrawElements(GL_TRIANGLES, m_skybox_vao.get_ibo_count(), GL_UNSIGNED_INT, NULL));
    }

    Framebuffer::bind_no_fbo();
    GL_CHECK(glBlitNamedFramebuffer(m_fbo_world.get_fbo_id(), 0, 0, 0, m_fbo_world.get_width(), m_fbo_world.get_height(), 0, 0, m_fbo_world.get_width(), m_fbo_world.get_height(), GL_COLOR_BUFFER_BIT, GL_LINEAR));
}

void Game::render_ui(void) {
    const float aspect = (float)m_fbo_world.get_width() / (float)m_fbo_world.get_height();

    m_fbo_ui.clear_fbo({ 0.1f, 0.1f, 0.1f, 1.0f }, CLEAR_COLOR | CLEAR_DEPTH | CLEAR_STENCIL);
    m_fbo_ui.bind_fbo();
    m_fbo_ui.set_viewport();
}

void Game::hotload_stuff(void) {
    m_block_shader.hotload();
    m_skybox_shader.hotload();
}

void Game::resize_framebuffers(int32_t new_width, int32_t new_height) {
    m_fbo_world.resize_fbo(new_width, new_height);
    m_fbo_ui.resize_fbo(new_width, new_height);
}

World &Game::get_world(void) {
    return m_world;
}

Camera &Game::get_camera(void) {
    return m_camera;
}

Player &Game::get_player(void) {
    return m_player;
}

void Game::push_debug_ui(void) {
    const vec3 camera_direction = m_camera.calc_direction();
    const vec3 camera_position  = m_camera.get_position();
    const vec2 camera_rotation  = m_camera.get_rotation();

    const vec3 player_position = m_player.get_position();
    const vec3 player_velocity = m_player.get_velocity();

    DebugUI::push_text_left(NULL);
    DebugUI::push_text_left(" --- World state ---");
    DebugUI::push_text_left("gen seed:    %d", m_world.get_seed());
    DebugUI::push_text_left("load radius: %d", g_load_radius);
    DebugUI::push_text_left("loaded chunks: %u", m_world.m_chunk_table.get_count());
    DebugUI::push_text_left("chunk buckets available: %u", m_world.m_chunk_table.get_size());
    DebugUI::push_text_left("chunk load queue: %d", m_world.m_load_queue.size());
    DebugUI::push_text_left("chunk gen  queue: %d", m_world.m_gen_queue.size());
    DebugUI::push_text_left("triangles rendered: %u (%.3fmln)", g_triangles_rendered_last_frame, (double)g_triangles_rendered_last_frame / 1000000.0);

    DebugUI::push_text_left(NULL);
    DebugUI::push_text_left(" --- Player ---");
    DebugUI::push_text_left("position: %.2f, %.2f, %.2f", player_position.x, player_position.y, player_position.z);
    DebugUI::push_text_left("velocity: %.2f, %.2f, %.2f", player_velocity.x, player_velocity.y, player_velocity.z);
    DebugUI::push_text_left("xz speed: %.3f", vec2::length(player_velocity.get_xz()));
    DebugUI::push_text_left("is_grounded: %s", BOOL_STR(m_player.is_grounded()));

    DebugUI::push_text_left(NULL);
    DebugUI::push_text_left(" --- Camera ---");
    DebugUI::push_text_left("fov:      %.2f", RAD_TO_DEG(m_camera.get_fov()));
    DebugUI::push_text_left("real:     %+.2f, %+.2f, %+.2f", camera_position.x, camera_position.y, camera_position.z);
    DebugUI::push_text_left("direction: %.3f, %.3f, %.3f", camera_direction.x, camera_direction.y, camera_direction.z);
    DebugUI::push_text_left("rotation H: %+.3f", RAD_TO_DEG(camera_rotation.x));
    DebugUI::push_text_left("rotation V: %+.3f", RAD_TO_DEG(camera_rotation.y));
}

void Game::add_console_commands(void) {
    Console::set_command("reset_world", { CONSOLE_COMMAND_LAMBDA {
            const int32_t seed = args.size() > 0 ? std::stoi(s_viu_to_std_string(args[0])) : game.get_world().get_seed();
            game.get_world().initialize_world(seed);
        }
    });

    Console::set_command("load_radius", { CONSOLE_COMMAND_LAMBDA {
            if(args.size() != 1) {
                return;
            }
            int32_t radius = std::stoi(s_viu_to_std_string(args[0]));
            if(radius) {
                g_load_radius = radius;
                char buffer[32];
                sprintf_s(buffer, 32, "> radius set to %d", radius);
                Console::add_to_history(buffer);
            }
        }
    });

    Console::set_command("toggle", { CONSOLE_COMMAND_LAMBDA {
            if(args.size() < 1) {
                Console::add_to_history("> Missing argument/s");
                return;
            }

            bool set_to;
            if(args[0] == "collider") {
                BOOL_TOGGLE(g_debug_show_player_collider);
                set_to = g_debug_show_player_collider;
            } else if(args[0] == "chunks") {
                BOOL_TOGGLE(g_debug_show_chunk_borders);
                set_to = g_debug_show_chunk_borders;
            } else if(args[0] == "wireframe") {
                BOOL_TOGGLE(g_debug_chunk_wireframe_mode);
                set_to = g_debug_chunk_wireframe_mode;
            } else {
                Console::add_to_history("> Invalid argument");
                return;
            }

            char buffer[64];
            sprintf_s(buffer, 64, "> Set to: %s", BOOL_STR(set_to));
            Console::add_to_history(buffer);
        }
    });

    Console::set_command("set_fov", { CONSOLE_COMMAND_LAMBDA {
            if(args.size() < 1) {
                Console::add_to_history("> missing argument");
                return;
            }
            float fov = std::stof(s_viu_to_std_string(args[0]));
            game.get_player().get_head_camera().set_fov(DEG_TO_RAD(fov));
        }
    });

    Console::set_command("spawn_cobble", { CONSOLE_COMMAND_LAMBDA {
            World &world = game.get_world();

            ChunkHashTable::Iterator iter = {};
            while(world.m_chunk_table.iterate_all(iter)) {
                for(int32_t x = 0; x < CHUNK_SIZE_X; ++x) {
                    for(int32_t y = 0; y < CHUNK_SIZE_Y; ++y) {
                        for(int32_t z = 0; z < CHUNK_SIZE_Z; ++z) {
                            Block *block = (*iter.value)->get_block({ x, y, z });
                            if(rand() % 100 == 0) {
                                block->set_type(BlockType::COBBLESTONE);
                            }
                        }
                    }
                }
                world.m_load_queue.push_back(iter.key);
                
                // @todo
                // (*iter.value)->queue_rebuild_vao();
            }
        }
    });
}
