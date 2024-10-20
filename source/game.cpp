#include "game.h"

#include "console.h"
#include "debug_ui.h"
#include "simple_draw.h"

#include <glew.h>

namespace {
    static int32_t g_load_radius = 8;
    static float   g_third_person_distance = 10.0f;

    static bool g_debug_show_chunk_borders  = false;
    static bool g_debug_show_player_collider = true;
    static bool g_debug_third_person_camera = true;
}

static void test_generate_world(World &world, int32_t size_half_x, int32_t size_half_z, int32_t seed) {
    world.initialize_world(seed);

    for(int32_t x = -size_half_x; x <= size_half_x; ++x) {
        for(int32_t z = -size_half_z; z <= size_half_z; ++z) {
            world.get_chunk({ x, z }, true);
        }
    }
}

constexpr vec3 init_player_pos = { 
    .x = CHUNK_SIZE_X * 0.5f,
    .y = 152.0f,
    .z = CHUNK_SIZE_Z * 0.5f
};

Game::Game(void) : m_world(this) {
    /* init resources */
    m_block_shader.set_filepath_and_load("C://dev//emce//source//shaders//block.glsl");
    m_block_atlas.load_from_file("C://dev//emce//data//atlas.png", true);
    m_block_atlas.set_filter_min(TextureFilter::NEAREST);
    m_block_atlas.set_filter_mag(TextureFilter::NEAREST);

    m_player.set_position(init_player_pos);
    m_camera.set_position(m_player.get_position());
    m_camera.set_rotation({ DEG_TO_RAD(90.0f), DEG_TO_RAD(-45.0f) });

    int32_t seed = 14; // rand() % RAND_MAX
    test_generate_world(m_world, 2, 2, seed);

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
                BOOL_TOGGLE(game.get_world()._debug_render_not_fill);
                set_to = game.get_world()._debug_render_not_fill;
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

Game::~Game(void) {
    /* free resources */
    m_block_shader.delete_shader();
    m_block_atlas.delete_texture_if_exists();
}

void Game::update(const Input &input, double delta_time) {
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

    if(!Console::is_open()) {
        if(input.key_pressed(Key::F05)) {
            BOOL_TOGGLE(g_debug_third_person_camera);
        }

        if(input.scroll_move()) {
            g_third_person_distance -= input.scroll_move();
            clamp_v(g_third_person_distance, 1.0f, 20.0f);
        }
    }

    const vec2i last_player_chunk = WorldPosition::from_real(m_player.get_position()).chunk;

    m_player.update(*this, input, delta_time);
    m_camera = m_player.get_head_camera();

    if(g_debug_third_person_camera) {
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


void Game::render(const Window &window) {
    GL_CHECK(glViewport(0, 0, window.get_width(), window.get_height()));
    m_block_shader.use_program();
    m_block_shader.upload_mat4("u_proj", m_camera.calc_proj(window.calc_aspect()).e);
    m_block_shader.upload_mat4("u_view", m_camera.calc_view().e);
    m_world.render_chunks(m_block_shader, m_block_atlas);

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

    const Camera head_camera = m_player.get_head_camera();
    const vec3 ray_origin = head_camera.get_position();
    const vec3 ray_end    = ray_origin + head_camera.calc_direction() * 8.0f;

    RaycastBlockResult raycast_result = raycast_block(m_world, ray_origin, ray_end);
    if(raycast_result.found) {
        WorldPosition p = raycast_result.block_p;
        WorldPosition next = WorldPosition::from_block(p.block + vec3i::make(raycast_result.normal));
        SimpleDraw::draw_cube_outline(raycast_result.intersection - vec3{ 0.25f, 0.25f, 0.25f } * 0.5f, { 0.25f, 0.25f, 0.25f }, 1.0f / 32.0f, { 1.0f, 1.0f, 0.0f, 1.0f });
        SimpleDraw::draw_cube_outline(next.real, { 1.0f, 1.0f, 1.0f }, 1.0f / 32.0f, { 0.8f, 0.8f, 0.8f, 0.8f });
    }

#if 0
    /* Test ray - plane intersection */ {
        const Camera head_camera = m_player.get_head_camera();
        const vec3 ray_origin = head_camera.get_position();
        const vec3 ray_end    = ray_origin + head_camera.calc_direction() * 130.0f;

        vec3 tri_a = { 1.21f, 200.0f, 5.0f };   
        vec3 tri_b = { 5.24f, 220.0f, 5.44f };   
        vec3 tri_c = { 3.18f, 214.4f, -10.2f };   

        vec3 plane_normal = vec3::cross(tri_b - tri_a, tri_c - tri_a);
        SimpleDraw::draw_line(tri_a, tri_a + plane_normal * 1.0f, 1.0f, { 1.0f, 1.0f, 0.0f, 1.0f });
        SimpleDraw::draw_line(tri_b, tri_b + plane_normal * 1.0f, 1.0f, { 1.0f, 1.0f, 0.0f, 1.0f });
        SimpleDraw::draw_line(tri_c, tri_c + plane_normal * 1.0f, 1.0f, { 1.0f, 1.0f, 0.0f, 1.0f });

        float k;
        vec3  p;
        if(ray_triangle_intersection(ray_origin, ray_end, tri_a, tri_b, tri_c, k, p)) {
            SimpleDraw::draw_cube_outline(p - vec3{ 0.25f, 0.25f, 0.25f } * 0.5f, { 0.25f, 0.25f, 0.25f }, 1.0f / 32.0f, { 1.0f, 1.0f, 0.0f, 1.0f });
        }

        SimpleDraw::draw_triangle(tri_a, tri_b, tri_c, { 0.2f, 0.85f, 0.3f, 1.0f });
    }
#endif
}

void Game::hotload_stuff(void) {
    m_block_shader.hotload();
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
    DebugUI::push_text_left("triangles rendered: %u (%.3fmln)", m_world._rendered_triangles_last_frame, (double)m_world._rendered_triangles_last_frame / 1000000.0);

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
