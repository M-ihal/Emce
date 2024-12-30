#include "game.h"
#include "window.h"

#define STRING_VIU_CPP_HELPERS
#include <string_viu.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_thread.h>

#include <vector>
#include <queue>

static void add_console_commands(Console &console);

Game::Game(void) : m_world(this) {
    /* Init debug state */
    debug_show_ui_info         = false;
    debug_show_chunk_borders   = false;
    debug_player_draw          = false;
    debug_chunk_wireframe_mode = false;
    debug_raycast_draw         = false;
    debug_spawn_p1             = { 0, 0, 0 };
    debug_spawn_p2             = { 200, 200, 200 };

    /* Init default game state */
    m_delta_time          = 0.0;
    m_elapsed_time        = 0.0;
    m_load_radius         = 24;
    m_3rd_person_mode     = false;
    m_3rd_person_distance = 10.0f;
    render_mode = GameRenderMode::NORMAL;

    /* Initialize threading resources */
    m_gen_chunks_condition = SDL_CreateCondition();
    m_gen_meshes_condition = SDL_CreateCondition();
    this->start_threads(1, 1);

    /* Initialize world state */
    m_world.initialize_new_world(5554);

    add_console_commands(m_console);
}

Game::~Game(void) {
    this->stop_threads();
    SDL_DestroyCondition(m_gen_chunks_condition);
    SDL_DestroyCondition(m_gen_meshes_condition);
}

void Game::set_load_radius(int32_t radius) {
    m_load_radius = radius;
}

GameFrameInfo Game::get_frame_info(void) {
    GameFrameInfo info;
    info.delta_time = m_delta_time;
    info.elapsed_time = m_elapsed_time;
    info.load_radius = m_load_radius;
    info.chunks_threads_active = m_gen_chunks_thread_num; 
    info.meshes_threads_active = m_gen_meshes_thread_num;
    info.is_3rd_person = m_3rd_person_mode;
    return info;
}

void Game::update(const Input &input, double delta_time) {
    m_delta_time = delta_time;
    m_elapsed_time += delta_time;

    Window &window = Window::get();

    /* Maybe close game */
    if(!m_console.is_open() && input.key_pressed(Key::ESCAPE)) {
        window.set_should_close();
        return;
    }

    /* Toggle debug ui */
    if(input.key_pressed(Key::F03)) {
        BOOL_TOGGLE(debug_show_ui_info);
    }

    /* Update Console */ {
        /* Open console */
        if(!m_console.is_open() && input.key_pressed(Key::T)) {
            m_console.set_open_state(true);
        }

        /* Close console */
        if(m_console.is_open() && input.key_pressed(Key::ESCAPE)) {
            m_console.set_open_state(false);
        }

        /* Set relative mouse mode if needed */
        if(!m_console.is_open() && !window.is_rel_mouse_active()) {
            window.set_rel_mouse_active(true);
        } else if(m_console.is_open() && window.is_rel_mouse_active()) {
            window.set_rel_mouse_active(false);
        }

        m_console.update(*this, input, delta_time);
    }

    /* Toggle third person camera */
    if(input.key_pressed(Key::F05) && !m_console.is_open()) {
        BOOL_TOGGLE(m_3rd_person_mode);
    }

    /* Zoom in/out third person camera */
    if(m_3rd_person_mode && (input.key_is_down(Key::PAGE_UP) || input.key_is_down(Key::PAGE_DOWN)) && !m_console.is_open()) {
        float dir = 0.0f;
        if(input.key_is_down(Key::PAGE_UP))   { dir -= 1.0; } 
        if(input.key_is_down(Key::PAGE_DOWN)) { dir += 1.0; }

        m_3rd_person_distance += dir * 30.0 * delta_time;
        clamp_v(m_3rd_person_distance, 1.0, 20.0);
    }

    /* Update player state */
    Player &player = m_world.get_player();
    player.update(*this, input, delta_time);

    /* Set render camera */
    if(m_3rd_person_mode) {
        m_camera = player.get_3rd_person_camera(m_3rd_person_distance);
    } else {
        m_camera = player.get_head_camera();
    }

    m_world.insert_generated_chunks();
    m_world.insert_generated_meshes();

    this->queue_new_chunks_to_load();
    this->update_loaded_chunks();
}

#define MAX_FRAME_CHUNK_DELETES 8
#define CHUNK_DELETE_RADIUS     2

void Game::update_loaded_chunks(void) {
    std::vector<Chunk *> chunks_to_delete;
    std::vector<Chunk *> chunks_to_mesh;
    chunks_to_delete.reserve(MAX_FRAME_CHUNK_DELETES);
    chunks_to_mesh.reserve(MAX_QUEUED_MESHES);

    double frustum[6][4];
    m_camera.calc_frustum_at_origin(frustum, Window::get().get_aspect());

    Player &player = m_world.get_player();

    ChunkHashTable::Iterator chunk_iter;
    while(m_world.iterate_chunks(chunk_iter)) {
        Chunk *chunk = chunk_iter.value;    

        vec2i chunk_coords = chunk->get_chunk_coords();
        vec3d chunk_absolute = real_position_from_chunk(chunk_coords);
        vec3d chunk_relative = m_camera.offset_to_relative(chunk_absolute);

        /* Maybe delete the chunk */
        bool should_delete = !is_chunk_in_range(chunk_coords, player.get_position_chunk(), m_load_radius + CHUNK_DELETE_RADIUS);
        if(should_delete) {
            chunks_to_delete.push_back(chunk);
            continue;
        }

        /* If waiting and neighbours got generated -> queue for meshing */
        const bool in_frustum = is_chunk_in_frustum(frustum, chunk_relative);
        if(chunk->should_build_mesh() && in_frustum && m_world.can_queue_mesh()) {
            chunks_to_mesh.push_back(chunk);
        }
    }

    /* Delete chunks @TODO : make proc that deletes chunk @TEMP */
    for(Chunk *chunk : chunks_to_delete) {
        vec2i chunk_coords = chunk->get_chunk_coords();
        m_world.delete_chunk(chunk_coords);
    }

    /* Sort chunks by distance from player's chunk @TODO : make it betta */
    std::sort(chunks_to_mesh.begin(), chunks_to_mesh.end(), [] (Chunk *a, Chunk *b) { 
        vec2 chunk = vec2::make(a->get_world()->get_player().get_position_chunk()); // @TODO
        float dist_a = vec2::length_sq(chunk - vec2::make(a->get_chunk_coords()));
        float dist_b = vec2::length_sq(chunk - vec2::make(b->get_chunk_coords()));
        return dist_a < dist_b; 
    });

    /* Queue the meshing */
    for(Chunk *chunk_to_mesh : chunks_to_mesh) {
        m_world.queue_build_mesh(chunk_to_mesh->get_chunk_coords());       
    }

    /* Push for meshing chunks outside frustum if slots available */
    ChunkHashTable::Iterator other_iter;
    while(m_world.can_queue_mesh() && m_world.iterate_chunks(other_iter)) {
        Chunk *chunk = other_iter.value;
        if(chunk->should_build_mesh()) {
            vec2i chunk_coords = chunk->get_chunk_coords();
            m_world.queue_build_mesh(chunk_coords);       
        }
    }
}

int32_t Game::thread_gen_chunks_proc(void) {
    int32_t ret_val = 0;

    for(; m_threads_keep_looping ;) {
        bool more_pending = m_world.generate_next_chunk();
        if(!more_pending) {
            /* If nothing to generate -> sleep thread */
            SDL_LockMutex(m_world.m_lock_chunk_gen);
            SDL_WaitCondition(m_gen_chunks_condition, m_world.m_lock_chunk_gen);
            SDL_UnlockMutex(m_world.m_lock_chunk_gen);
        }
    }
    return ret_val;
}

int32_t Game::thread_gen_meshes_proc(void) {
    int32_t ret_val = 0;

    for(; m_threads_keep_looping ;) {
        bool more_pending = m_world.generate_next_mesh();
        if(!more_pending) {
            /* If nothing to generate -> sleep thread */
            SDL_LockMutex(m_world.m_lock_mesh_gen);
            SDL_WaitCondition(m_gen_meshes_condition, m_world.m_lock_mesh_gen);
            SDL_UnlockMutex(m_world.m_lock_mesh_gen);
        }
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

void Game::queue_new_chunks_to_load(void) {
    for(int i = 0; i < 2; ++i) {
        for(int32_t j = -i; j <= i; ++j) {
            for(int32_t k = -i; k <= i; ++k) {
                if(!m_world.can_queue_chunk()) {
                    return;
                }

                vec2i chunk_coords = m_world.get_player().get_position_chunk() + vec2i{ j, k };
                m_world.queue_create_chunk(chunk_coords);
            }
        }
    }

    for(int32_t i = 0; i < m_load_radius + 1; ++i) {
        for(int32_t j = -i; j <= i; ++j) {
            for(int32_t k = -i; k <= i; ++k) {
                if(!m_world.can_queue_chunk()) {
                    return;
                }

                vec2i chunk_coords = m_world.get_player().get_position_chunk() + vec2i{ j, k };
                m_world.queue_create_chunk(chunk_coords);
            }
        }
    }
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

CONSOLE_COMMAND_PROC(command_threads) {
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

CONSOLE_COMMAND_PROC(command_generate) {
    World &world = game.get_world();
    GameWorldInfo world_info = world.get_world_info();

    int32_t seed;
    if(args.size() == 0) {
        seed = world_info.world_gen_seed;
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

CONSOLE_COMMAND_PROC(command_load_radius) {
    if(args.size() != 1) {
        console.add_to_history("Invalid number of arguments, 1 required.");
        return;
    }

    int32_t radius = std::stoi(s_viu_to_std_string(args[0]));
    if(radius <= 0) {
        console.add_to_history("Invalid load radius number.");
        return;
    }

    game.set_load_radius(radius);
}

CONSOLE_COMMAND_PROC(command_render_mode) {
    if(args.size() != 1) {
        console.add_to_history("Invalid number of arguments, 1 required.");
        return;
    }

    if(args[0] == "normal") {
        game.render_mode = GameRenderMode::NORMAL;
    } else if(args[0] == "normals") {
        game.render_mode = GameRenderMode::DEBUG_NORMALS;
    } else if(args[0] == "depth") {
        game.render_mode = GameRenderMode::DEBUG_DEPTH;
    } else if(args[0] == "AO") {
        game.render_mode = GameRenderMode::DEBUG_AO;
    } else {
        console.add_to_history("Invalid argument.");
    }
}

CONSOLE_COMMAND_PROC(command_toggle) {
    if(args.size() != 1) {
        console.add_to_history("Invalid number of arguments, 1 required.");
        return;
    }

    if(args[0] == "player") {
        BOOL_TOGGLE(game.debug_player_draw);
    } else if(args[0] == "chunks") {
        BOOL_TOGGLE(game.debug_show_chunk_borders);
    } else if(args[0] == "wireframe") {
        BOOL_TOGGLE(game.debug_chunk_wireframe_mode);
    } else if(args[0] == "raycast") {
        BOOL_TOGGLE(game.debug_raycast_draw);
    } else {
        console.add_to_history("Invalid toggle argument.");
    }
}

CONSOLE_COMMAND_PROC(command_fov) {
    if(args.size() != 1) {
        console.add_to_history("Invalid number of arguments, 1 required.");
        return;
    }

    float fov = std::stof(s_viu_to_std_string(args[0]));
    if(fov <= 1.0) {
        console.add_to_history("Invalid FOV value.");
        return;
    }
    
    Player &player = game.get_world().get_player();
    player.set_head_camera_fov(DEG_TO_RAD(fov));
}

CONSOLE_COMMAND_PROC(command_goto) {
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

static void add_console_commands(Console &console) {
    console.set_command("threads",         command_threads);
    console.set_command("generate",        command_generate);
    console.set_command("load_radius",     command_load_radius);
    console.set_command("render_mode",     command_render_mode);
    console.set_command("toggle",          command_toggle);
    console.set_command("fov",             command_fov);
    console.set_command("goto",            command_goto);
    console.set_command("fly",             command_fly);
}

