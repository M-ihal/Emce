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

    Console::register_command({
        .command = "p",
        .proc = CONSOLE_COMMAND_LAMBDA {
            if(args.size()) {
                Console::add_to_history("> not implemented");
            } else {
                game.get_player().set_position(init_player_pos);
            }
        }
    });

    m_player.set_position(init_player_pos);

    m_camera.set_position(m_player.get_position());
    m_camera.set_rotation({ DEG_TO_RAD(90.0f), DEG_TO_RAD(-45.0f) });

    int32_t seed = 14; // rand() % RAND_MAX
    test_generate_world(m_world, 2, 2, seed);

    Console::register_command({
        .command = "generate",
        .proc = CONSOLE_COMMAND_LAMBDA {
            if(args.size() < 3) {
                Console::add_to_history("> missing arguments");
                return;
            }
            int32_t half_x = std::stoi(args[0]);
            int32_t half_y = std::stoi(args[1]);
            int32_t seed = std::stoi(args[2]);
            half_x = clamp(half_x, 0, 100);
            half_y = clamp(half_y, 0, 100);
            test_generate_world(game.get_world(), half_x, half_y, seed);
        }
    });

    Console::register_command({
        .command = "reset_world",
        .proc = CONSOLE_COMMAND_LAMBDA {
            const int32_t seed = args.size() > 0 ? std::stoi(args[0]) : game.get_world().get_seed();
            game.get_world().initialize_world(seed);
        }
    });

    Console::register_command({
        .command = "setrad",
        .proc = CONSOLE_COMMAND_LAMBDA {
            if(args.size() != 1) {
                return;
            }
            int32_t radius = std::stoi(args[0]);
            if(radius) {
                g_load_radius = radius;
                char buffer[32];
                sprintf_s(buffer, 32, "> radius set to %d", radius);
                Console::add_to_history(buffer);
            }
        }
    });

    Console::register_command({
        .command = "debug_toggle",
        .proc = CONSOLE_COMMAND_LAMBDA {
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

    Console::register_command({
        .command = "fov",
        .proc = CONSOLE_COMMAND_LAMBDA {
            if(args.size() < 1) {
                Console::add_to_history("> missing argument");
                return;
            }
            float fov = std::stof(args[0]);
            game.get_player().get_head_camera().set_fov(DEG_TO_RAD(fov));
        }
    });
}

Game::~Game(void) {
    /* free resources */
    m_block_shader.delete_shader();
    m_block_atlas.delete_texture_if_exists();
}

void Game::update(const Input &input, double delta_time) {
    const vec2i last_player_chunk = WorldPosition::from_real(m_player.get_position()).chunk;
    m_player.update(*this, input, delta_time);

    if(last_player_chunk != WorldPosition::from_real(m_player.get_position()).chunk) {
        m_world.m_should_sort_load_queue = true;
    }

    m_camera = m_player.get_head_camera();

    if(!Console::is_open()) {

        if(input.key_pressed(Key::F05)) {
            BOOL_TOGGLE(g_debug_third_person_camera);
        }

        if(input.scroll_move()) {
            g_third_person_distance -= input.scroll_move();
            clamp_v(g_third_person_distance, 1.0f, 20.0f);
        }
    }

    if(g_debug_third_person_camera) {
        vec3 dir = m_camera.calc_direction();
        m_camera.set_position(m_camera.get_position() - dir * g_third_person_distance);
    }

    WorldPosition camera_position = WorldPosition::from_real(m_camera.get_position());
    const vec3 camera_direction = m_camera.calc_direction();
    const vec2 camera_rotation = m_camera.get_rotation();

    DebugUI::push_text_left(NULL);
    DebugUI::push_text_left(" --- Camera ---");
    DebugUI::push_text_left("fov:      %.2f", RAD_TO_DEG(m_camera.get_fov()));
    DebugUI::push_text_left("real:     %+.2f, %+.2f, %+.2f", camera_position.real.x, camera_position.real.y, camera_position.real.z);
    DebugUI::push_text_left("block:    %+d, %+d, %+d", camera_position.block.x, camera_position.block.y, camera_position.block.z);
    DebugUI::push_text_left("blockrel: %+d, %+d, %+d", camera_position.block_rel.x, camera_position.block_rel.y, camera_position.block_rel.z);
    DebugUI::push_text_left("chunk:    %+d, %+d", camera_position.chunk.x, camera_position.chunk.y);
    DebugUI::push_text_left("direction: %.3f, %.3f, %.3f", camera_direction.x, camera_direction.y, camera_direction.z);
    DebugUI::push_text_left("rotation H: %+.3f", RAD_TO_DEG(camera_rotation.x));
    DebugUI::push_text_left("rotation V: %+.3f", RAD_TO_DEG(camera_rotation.y));

    DebugUI::push_text_right("world seed: %d", m_world.get_seed());
    DebugUI::push_text_right("number of chunks: %d", m_world.get_chunk_map_size());

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

    m_world.process_load_queue();
    m_world.process_gen_queue();

    DebugUI::push_text_right("chunk load queue: %d", m_world.m_load_queue.size());
    DebugUI::push_text_right("chunk gen  queue: %d", m_world.m_gen_queue.size());
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
        const Color ground_collider_color = m_player.check_is_grounded(m_world) ? Color{ 0.0f, 1.0f, 0.0f, 1.0f } : Color{ 1.0f, 0.0f, 0.0f, 1.0f };
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
        for(auto const &[key, chunk] : m_world.get_chunk_map()) {
            vec3 chunk_pos = { 
                float(chunk->get_coords().x * CHUNK_SIZE_X),
                0.0f,
                float(chunk->get_coords().y * CHUNK_SIZE_Z)
            };
            SimpleDraw::draw_cube_outline(chunk_pos, { CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z }, 8.0f / 16.0f, { 1.0f, 1.0f, 1.0f, 1.0f });
        }
    }
}

void Game::hotload_stuff(void) {
    m_block_shader.hotload();
}

World &Game::get_world(void) {
    return m_world;
}

const World &Game::get_world(void) const {
    return m_world;
}

Camera &Game::get_camera(void) {
    return m_camera;
}

const Camera &Game::get_camera(void) const {
    return m_camera;
}

Player &Game::get_player(void) {
    return m_player;
}

const Player &Game::get_player(void) const {
    return m_player;
}

