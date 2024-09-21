#include "game.h"

#include "console.h"
#include "debug_ui.h"

#include <glew.h>

namespace {
    static int32_t s_load_radius = 8;
}

// @temp
static void test_generate_world(World &world, int32_t size_half_x, int32_t size_half_z, int32_t seed) {
    world.delete_chunks();
    world.set_seed(seed);
    for(int32_t x = -size_half_x; x <= size_half_x; ++x) {
        for(int32_t z = -size_half_z; z <= size_half_z; ++z) {
            world.get_chunk(x, z, true);
        }
    }
}

Game::Game(void) {
    /* init resources */
    m_block_shader.set_filepath_and_load("C://dev//emce//source//shaders//block.glsl");
    m_block_atlas.load_from_file("C://dev//emce//data//atlas.png", true);
    m_block_atlas.set_filter_min(TextureFilter::NEAREST);
    m_block_atlas.set_filter_mag(TextureFilter::NEAREST);

    m_camera.set_position({ -54.0f, 128.0f, 37.0f });
    m_camera.set_rotation({ DEG_TO_RAD(90.0f), DEG_TO_RAD(-45.0f) });

    test_generate_world(m_world, 10, 10, 23155);

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
        .command = "setrad",
        .proc = CONSOLE_COMMAND_LAMBDA {
            if(args.size() != 1) {
                return;
            }
            int32_t radius = std::stoi(args[0]);
            if(radius) {
                s_load_radius = radius;
                char buffer[32];
                sprintf_s(buffer, 32, "> radius set to %d", radius);
                Console::add_to_history(buffer);
            }
        }
    });
}

Game::~Game(void) {
    /* free resources */
    m_block_shader.delete_shader();
    m_block_atlas.delete_texture_if_exists();
}

static void update_game_camera(Camera &camera, const Input &input, double delta_time) {
    if(Console::is_open()) {
        return;
    }

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

void Game::update(const Input &input, double delta_time) {
    update_game_camera(m_camera, input, delta_time);

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

    DebugUI::push_text_right("number of chunks: %d", m_world.m_chunks.size());

    // m_world.get_chunk(camera_position.chunk.x, camera_position.chunk.y, true);
}

void Game::render(const Window &window) {
    GL_CHECK(glViewport(0, 0, window.width(), window.height()));
    m_block_shader.use_program();
    m_block_shader.upload_mat4("u_proj", m_camera.calc_proj(window.calc_aspect()).e);
    m_block_shader.upload_mat4("u_view", m_camera.calc_view().e);
    m_world.render_chunks(m_block_shader, m_block_atlas);
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

vec3i block_position_from_real(const vec3 &real) {
    return {
        (int32_t)floorf(real.x),
        (int32_t)floorf(real.y),
        (int32_t)floorf(real.z),
    };
}

vec2i chunk_position_from_block(const vec3i &block) {
    return {
        (int32_t)floorf(float(block.x) / CHUNK_SIZE_X),
        (int32_t)floorf(float(block.z) / CHUNK_SIZE_Z)
    };
}

vec3i block_relative_from_block(const vec3i &block) {
    vec3i rel = {
        block.x % CHUNK_SIZE_X,
        block.y,
        block.z % CHUNK_SIZE_Z,
    };

    if(block.x < 0) {
        rel.x = CHUNK_SIZE_X + rel.x;
    }
    if(block.z < 0) {
        rel.z = CHUNK_SIZE_Z + rel.z;
    }

    return rel;
}

WorldPosition WorldPosition::from_real(const vec3 &real) {
    WorldPosition result = { };
    result.real = real;
    result.block = block_position_from_real(real);
    result.block_rel = block_relative_from_block(result.block);
    result.chunk = chunk_position_from_block(result.block);
    return result;
}
