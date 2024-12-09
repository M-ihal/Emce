#include "game.h"
#include "window.h"
#include "opengl_abs.h"

#define STRING_VIU_CPP_HELPERS
#include <string_viu.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_thread.h>

#include <cstdarg>

Game::Game(void) : m_world(this) {
    /* Init debug state */
    debug_show_ui_info         = false;
    debug_show_chunk_borders   = false;
    debug_player_draw          = false;
    debug_chunk_wireframe_mode = false;
    debug_raycast_draw         = false;
    debug_spawn_p1             = { 0, 0, 0 };
    debug_spawn_p2             = { 200, 200, 200 };

    render_mode = GameRenderMode::NORMAL;

    /* Init default game state */
    m_load_radius         = 10;
    m_3rd_person_mode     = false;
    m_3rd_person_distance = 10.0f;

    /* Init - Resize game */
    Window &window = Window::get();
    // this->resize(window.get_width(), window.get_height());

    /* Init global mesh gen state */
    chunk_mesh_gen_data_init_global();

    /* Initialize threading resources */
    m_gen_chunks_condition = SDL_CreateCondition();
    m_gen_meshes_condition = SDL_CreateCondition();
    this->start_threads(1, 1);

    /* Initialize world state */
    m_world.initialize_new_world(5554);

    /* Initialize console and add some commands */
    m_console.initialize();
    this->add_console_commands();
}

Game::~Game(void) {
    this->stop_threads();
    SDL_DestroyCondition(m_gen_chunks_condition);
    SDL_DestroyCondition(m_gen_meshes_condition);

    chunk_mesh_gen_data_free_global();

    m_console.destroy();
}

void Game::update(const Input &input, double delta_time) {
    m_delta_time = delta_time;
    m_time_elapsed += delta_time;

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

    /* Queue generation of new chunks */
 //   if(player.moved_chunk_last_frame()) {
         this->queue_new_chunks_to_load();
   // }

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

#if 0
        /* If chunk's neighbours are generated -> queue mesh generation, otherwise -> set waiting state */
        bool has_neighbours = m_world.chunk_neighbours_generated(chunk->get_chunk_xz());
        if(has_neighbours) {
            m_world.gen_chunk_mesh_offload(chunk->get_chunk_xz());
            chunk->set_mesh_state(ChunkMeshState::QUEUED);
        } else {
            chunk->set_mesh_state(ChunkMeshState::WAITING);
        }
#endif
            chunk->set_mesh_state(ChunkMeshState::WAITING);
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
        if(mesh_data->has_been_dropped == false && chunk != NULL && chunk->get_mesh_state() != ChunkMeshState::LOADED) {
            chunk->set_mesh(mesh_data);
            chunk->set_mesh_state(ChunkMeshState::LOADED);
        }

        chunk_mesh_gen_data_free(&mesh_data);
    }

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

        vec2i chunk_coords = chunk->get_chunk_xz();
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
        if(chunk->should_build_mesh() && in_frustum && chunk_mesh_slots_available()) {
            chunks_to_mesh.push_back(chunk);
        }
    }

    /* Delete chunks @TODO : make proc that deletes chunk @TEMP */
    for(Chunk *chunk : chunks_to_delete) {
        vec2i chunk_coords = chunk->get_chunk_xz();
        m_world.delete_chunk(chunk_coords);
    }

    /* Sort chunks by distance from player's chunk @TODO : make it betta */
    std::sort(chunks_to_mesh.begin(), chunks_to_mesh.end(), [] (Chunk *a, Chunk *b) { 
        vec2 chunk = vec2::make(a->get_world()->get_player().get_position_chunk()); // @TODO
        float dist_a = vec2::length_sq(chunk - vec2::make(a->get_chunk_xz()));
        float dist_b = vec2::length_sq(chunk - vec2::make(b->get_chunk_xz()));
        return dist_a < dist_b; 
    });

    /* Queue the meshing */
    for(Chunk *chunk_to_mesh : chunks_to_mesh) {
        m_world.gen_chunk_mesh_offload(chunk_to_mesh->get_chunk_xz());       
    }

    /* Push for meshing chunks outside frustum if slots available */
    ChunkHashTable::Iterator other_iter;
    while(chunk_mesh_slots_available() && m_world.iterate_chunks(other_iter)) {
        Chunk *chunk = other_iter.value;
        if(chunk->should_build_mesh()) {
            vec2i chunk_coords = chunk->get_chunk_xz();
            m_world.gen_chunk_mesh_offload(chunk_coords);       
        }
    }
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
            if(!is_chunk_in_range(m_world.get_player().get_position_chunk(), mesh_data->chunk_xz, m_load_radius)) {
                /* Too far, do not generate */
                mesh_data->has_been_dropped = true;
                SDL_LockMutex(m_world.m_lock_mesh_gen);
                m_world.m_meshes_built.push_back(mesh_data);
                SDL_UnlockMutex(m_world.m_lock_mesh_gen);
            } else {
                /* Do the mesh generation */
                chunk_mesh_gen(mesh_data);
                mesh_data->has_been_dropped = false;

                SDL_LockMutex(m_world.m_lock_mesh_gen);
                m_world.m_meshes_built.push_back(mesh_data);
                SDL_UnlockMutex(m_world.m_lock_mesh_gen);
            }
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

void Game::queue_new_chunks_to_load(void) {
    for(int i = 0; i < 2; ++i) {
        for(int32_t j = -i; j <= i; ++j) {
            for(int32_t k = -i; k <= i; ++k) {
                vec2i chunk_xz = m_world.get_player().get_position_chunk() + vec2i{ j, k };
                m_world.create_chunk_offload(chunk_xz);
            }
        }
    }

    for(int32_t i = 0; i < m_load_radius + 1; ++i) {
        for(int32_t j = -i; j <= i; ++j) {
            for(int32_t k = -i; k <= i; ++k) {

                vec2i chunk_xz = m_world.get_player().get_position_chunk() + vec2i{ j, k };
                m_world.create_chunk_offload(chunk_xz);
                // }
            }
        }
    }
no_slots:;
    // m_world.create_chunks_in_range_offload(m_world.get_player().get_position_chunk(), g_load_radius + 1);
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
    // game.check_for_chunks_to_load();
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

    ChunkHashTable::Iterator iter;
    while(world.iterate_chunks(iter)) {
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
    game.debug_spawn_p1 = vec3i{ x, y, z };
}

CONSOLE_COMMAND_PROC(command_spawn_set_point_2) {
    if(args.size() != 3) {
        console.add_to_history("Invalid number of arguments, 3 required.");
        return;
    }

    int32_t x = std::stoi(s_viu_to_std_string(args[0]));
    int32_t y = std::stoi(s_viu_to_std_string(args[1]));
    int32_t z = std::stoi(s_viu_to_std_string(args[2]));
    game.debug_spawn_p2 = vec3i{ x, y, z };
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
        MIN(game.debug_spawn_p1.x, game.debug_spawn_p2.x),
        MIN(game.debug_spawn_p1.y, game.debug_spawn_p2.y),
        MIN(game.debug_spawn_p1.z, game.debug_spawn_p2.z),
    };

    vec3i max = {
        MAX(game.debug_spawn_p1.x, game.debug_spawn_p2.x),
        MAX(game.debug_spawn_p1.y, game.debug_spawn_p2.y),
        MAX(game.debug_spawn_p1.z, game.debug_spawn_p2.z),
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

SDL_EnumerationResult enumerate_load_chunk(void *userdata, const char *dir_name, const char *name) {
    World *world = static_cast<World *>(userdata);

    StringViu name_view = s_viu(name);
    StringViu name_no_ext;
    if(s_viu_ends_with(name_view, &name_no_ext, ".chunk")) {
        int32_t x, z;
        sscanf_s(name_no_ext.data, "%d_%d", &x, &z);

        char buffer[32];
        sprintf_s(buffer, 32, "save_data/%d_%d.chunk", x, z);

        FileContents file;
        if(read_entire_file(buffer, file)) {
            Chunk *chunk = world->get_chunk_create({ x, z }, (BlockType *)file.data);
            chunk->set_mesh_state(ChunkMeshState::WAITING);
        }
    }
    return SDL_ENUM_CONTINUE;
}

void Game::add_console_commands(void) {
    m_console.set_command("threads",         command_threads);
    m_console.set_command("generate",        command_generate);
    m_console.set_command("load_radius",     command_load_radius);
    m_console.set_command("render_mode",     command_render_mode);
    m_console.set_command("toggle",          command_toggle);
    m_console.set_command("fov",             command_fov);
    m_console.set_command("spawn_stuff",     command_spawn_stuff);
    m_console.set_command("spawn_set_p1",    command_spawn_set_point_1);
    m_console.set_command("spawn_set_p2",    command_spawn_set_point_2);
    m_console.set_command("spawn",           command_spawn);
    m_console.set_command("goto",            command_goto);
    m_console.set_command("fly",             command_fly);
}

