#include "world.h"
#include "game.h"
#include "chunk_gen.h"

#include <algorithm>
#include <glew.h>

/*
 * TODO
 * 3 TYPES OF GENERATING MESH
 * INITIAL GENERATION
 * CHANGE GENERATION
 *
 * IF NOT INITIAL AND THEN CHANGE - SKIP INITIAL, DO CHANGE
 * IF INITIAL THEN CHANGE - DO CHANGE
 * 
 *
 * */

Game *World::get_game(void) {
    ASSERT(m_owner != NULL && "World without owner error!.");
    return m_owner;
}

Player &World::get_player(void) {
    return m_player;
}

World::World(Game *game) {
    m_owner = game;
    m_lock_chunk_gen = SDL_CreateMutex();
    m_lock_mesh_gen  = SDL_CreateMutex();
    m_chunk_table.initialize_table(5000);
    m_gen_seed.seed = 0;
}

World::~World(void) {
    this->delete_chunks();
    SDL_DestroyMutex(m_lock_chunk_gen);
    SDL_DestroyMutex(m_lock_mesh_gen);
}

void World::initialize_new_world(uint32_t seed) {
    this->delete_chunks();
    m_gen_seed.seed = seed;

    /* Create spawn chunk before initializing player */
    const vec2i spawn_chunk = { 0, 0 };
    this->get_chunk_create(spawn_chunk);

    /* Setup player state and position */
    m_player.setup_player(*this, spawn_chunk);
}

void World::delete_chunks(void) {
    m_chunk_gen_queue.clear();
    m_mesh_gen_queue.clear();
    m_mesh_gen_queue_high_prio.clear();

    ChunkHashTable::Iterator iter;
    while(m_chunk_table.iterate_all(iter)) {
        Chunk *chunk = iter.value;
        delete chunk;
    }
    m_chunk_table.clear_table();
}

void World::delete_chunk(vec2i chunk_coords) {
    Chunk **chunk_hash = m_chunk_table.find(chunk_coords);
    if(chunk_hash == NULL) {
        /* Does not exist? */
        return;
    }

    /* Free memory */
    Chunk *chunk = *chunk_hash;
    ASSERT(chunk);
    delete chunk;

    m_chunk_table.remove(chunk_coords);
}

Chunk *World::get_chunk(vec2i chunk_xz) {
    Chunk **chunk_hash = m_chunk_table.find(chunk_xz);
    if(chunk_hash != NULL) {
        Chunk *chunk = *chunk_hash;
        ASSERT(chunk);
        if(chunk->get_chunk_state() == ChunkState::GENERATING) {
            return NULL;
        } else {
            return chunk;
        }
    }
    return NULL;
}

Chunk *World::get_chunk_create(vec2i chunk_xz, BlockType blocks[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z]) {
    Chunk *to_generate = NULL;
    Chunk **chunk_hash = m_chunk_table.find(chunk_xz);
    if(chunk_hash != NULL) {
        Chunk *chunk = *chunk_hash;
        if(chunk->get_chunk_state() == ChunkState::GENERATED) {
            return chunk;
        } else {
            to_generate = chunk;
        }
    } else {
        to_generate = new Chunk(this, chunk_xz);
        m_chunk_table.insert(chunk_xz, to_generate);
    }

    if(to_generate != NULL) {
        if(blocks) {
            to_generate->copy_blocks_into(blocks);
        } else {
            ChunkGenData gen_data;
            chunk_gen_data_init(gen_data, chunk_xz, m_gen_seed);
            chunk_gen(gen_data);

            to_generate->copy_blocks_into(gen_data.blocks);
        }

        to_generate->set_chunk_state(ChunkState::GENERATED);
        to_generate->set_mesh_state(ChunkMeshState::WAIT_FOR_MESHING);
    }
    return to_generate;
}

BlockType World::get_block(vec3i block_abs, Chunk **out_chunk) {
    WorldPosition block_pos = WorldPosition::from_block(block_abs);

    Chunk *chunk = this->get_chunk(block_pos.chunk);
    if(chunk == NULL) {
        return BlockType::_INVALID;
    }

    BlockType block = chunk->get_block(block_pos.block_rel);
    if(out_chunk != NULL) {
        *out_chunk = chunk;
    }
    return block;
}

void World::queue_create_chunk(vec2i chunk_xz) {
    Chunk **chunk_hash = m_chunk_table.find(chunk_xz);
    if(chunk_hash != NULL) {
        return;
    }

    ChunkGenData *gen_data = m_chunk_gen_queue.get_free_element();
    if(gen_data == NULL) {
        return;
    }

    Chunk *chunk = new Chunk(this, chunk_xz);
    m_chunk_table.insert(chunk_xz, chunk);

    chunk_gen_data_init(*gen_data, chunk_xz, m_gen_seed);

    SDL_LockMutex(m_lock_chunk_gen);
    m_chunk_gen_queue.in_queue.push(gen_data);
    SDL_UnlockMutex(m_lock_chunk_gen);

    m_owner->wake_up_gen_chunks_threads();
}

void World::queue_build_mesh(vec2i chunk_coords, bool high_prio) {
    Chunk *chunk = this->get_chunk(chunk_coords);
    if(chunk == NULL) {
        /* The chunk isn't loaded ... */
        return;
    }

    StaticGenQueue<ChunkMeshGenData, MAX_QUEUED_MESHES> &queue = high_prio ? m_mesh_gen_queue_high_prio : m_mesh_gen_queue;

    ChunkMeshGenData *mesh_data = queue.get_free_element();
    if(mesh_data == NULL) {
        return;
    }

    chunk_mesh_gen_data_init(mesh_data, *this, chunk_coords);

    SDL_LockMutex(m_lock_mesh_gen);
    queue.in_queue.push(mesh_data);
    SDL_UnlockMutex(m_lock_mesh_gen);

    chunk->set_mesh_state(ChunkMeshState::QUEUED);

    m_owner->wake_up_gen_meshes_threads();
}

void World::rebuild_mesh_slow(vec2i chunk_coords) {
    Chunk *chunk = this->get_chunk(chunk_coords);
    if(chunk == NULL) {
        /* The chunk isn't loaded ... */
        return;
    }

    /* Can't build mesh if neighbours are not generated */
    if(!this->chunk_neighbours_generated(chunk_coords)) {
        return;
    }

    ChunkMeshGenData *mesh_data = new ChunkMeshGenData();
    chunk_mesh_gen_data_init(mesh_data, *this, chunk_coords);

    chunk_mesh_gen(mesh_data);
    chunk->set_mesh(mesh_data);
    chunk->set_mesh_state(ChunkMeshState::COMPLETE);

    delete mesh_data;
}

void World::set_chunk_should_rebuild_mesh(vec2i chunk_coords, bool important) {
    Chunk *chunk = this->get_chunk(chunk_coords);
    if(chunk == NULL) {
        return;
    }

    if(important) {
        chunk->set_mesh_state(ChunkMeshState::WAIT_FOR_MESHING_HIGH_PRIORITY);
    } else {
        chunk->set_mesh_state(ChunkMeshState::WAIT_FOR_MESHING);
    }
}

bool World::chunk_neighbours_generated(vec2i chunk_xz) {
    Chunk *neighbours[8] = {
        this->get_chunk(chunk_xz + vec2i{ -1,  0 }),
        this->get_chunk(chunk_xz + vec2i{ +1,  0 }),
        this->get_chunk(chunk_xz + vec2i{  0, -1 }),
        this->get_chunk(chunk_xz + vec2i{  0, +1 }),
        this->get_chunk(chunk_xz + vec2i{ -1, -1 }),
        this->get_chunk(chunk_xz + vec2i{ +1, +1 }),
        this->get_chunk(chunk_xz + vec2i{ -1, +1 }),
        this->get_chunk(chunk_xz + vec2i{ +1, -1 }),
    };

    return (neighbours[0] && neighbours[1] && neighbours[2] && neighbours[3]
         && neighbours[4] && neighbours[5] && neighbours[6] && neighbours[7]);
}

GameWorldInfo World::get_world_info(void) {
    GameWorldInfo info;
    info.world_gen_seed = m_gen_seed.seed;
    info.chunks_loaded = m_chunk_table.get_count();
    info.chunks_allocated = m_chunk_table.get_size();
    info.chunks_queued = m_chunk_gen_queue.in_queue.size;
    info.meshes_queued = m_mesh_gen_queue.in_queue.size;
    info.meshes_queued_high_prio = m_mesh_gen_queue_high_prio.in_queue.size;
    return info;
}

bool World::iterate_chunks(ChunkHashTable::Iterator &iter) {
    while(true) {
        bool ret_val = m_chunk_table.iterate_all(iter);
        if(ret_val) {
            Chunk *chunk = iter.value;
            if(chunk->get_chunk_state() != ChunkState::GENERATED) {
                /* If this chunk is still generating, skip it */
                continue;
            } else {
                return true;
            }
        } else {
            return false;
        }
    }
}

void World::insert_generated_chunks(void) {
    while(true) {
        ChunkGenData *gen_data;

        SDL_LockMutex(m_lock_mesh_gen);
        bool exists = m_chunk_gen_queue.finished.pop(&gen_data);
        SDL_UnlockMutex(m_lock_mesh_gen);

        /* No more generated chunks */
        if(!exists) {
            break;
        }

        Chunk **chunk_hash = m_chunk_table.find(gen_data->chunk_xz);
        if(!chunk_hash) {
            m_chunk_gen_queue.release_element(gen_data);
            continue;
        }

        Chunk *chunk = *chunk_hash;
        if(chunk->get_chunk_state() == ChunkState::GENERATED) {
            m_chunk_gen_queue.release_element(gen_data);
            /* For some reason, queued chunk already has been generated... ignore it */
            continue;
        }

        chunk->copy_blocks_into(gen_data->blocks);
        chunk->set_chunk_state(ChunkState::GENERATED);
        chunk->set_mesh_state(ChunkMeshState::WAIT_FOR_MESHING);

        m_chunk_gen_queue.release_element(gen_data);
    }
}

void World::insert_generated_meshes(void) {
    while(true) {
        ChunkMeshGenData *mesh_data = NULL;

        SDL_LockMutex(m_lock_mesh_gen);
        StaticGenQueue<ChunkMeshGenData, MAX_QUEUED_MESHES> &queue = m_mesh_gen_queue_high_prio.finished.is_empty() ? m_mesh_gen_queue : m_mesh_gen_queue_high_prio;
        bool exists = queue.finished.pop(&mesh_data);
        SDL_UnlockMutex(m_lock_mesh_gen);

        /* No more built meshes */
        if(!exists) {
            break;
        }

        Chunk *chunk = this->get_chunk(mesh_data->chunk_coords);

        /* if !chunk -> Chunk has been deleted during the mesh generation so ignore it */
        /* Set generated mesh if the state isn't LOADED (could happen if immediatelly loading later) */
        if(chunk && !mesh_data->has_been_dropped && mesh_data->chunk_mesh_build_id == chunk->get_mesh_build_counter()) {
            chunk->set_mesh(mesh_data);
            chunk->set_mesh_state(ChunkMeshState::COMPLETE);
        }

        queue.release_element(mesh_data);
    }
}

bool World::generate_next_chunk(void) {
    ChunkGenData *gen_data;

    /* Get next chunk to generate */
    SDL_LockMutex(m_lock_chunk_gen);
    bool exists = m_chunk_gen_queue.in_queue.pop(&gen_data);
    SDL_UnlockMutex(m_lock_chunk_gen);

    /* No chunk to generate */
    if(!exists) {
        return false;
    }

    /* Do the chunk generation */
    chunk_gen(*gen_data);

    /* Push generated chunk to insert array */
    SDL_LockMutex(m_lock_chunk_gen);
    m_chunk_gen_queue.finished.push(gen_data);
    SDL_UnlockMutex(m_lock_chunk_gen);

    return !m_chunk_gen_queue.in_queue.is_empty();
}

bool World::generate_next_mesh(void) {
    ChunkMeshGenData *mesh_data;

    /* Get next queued mesh to build */
    SDL_LockMutex(m_lock_mesh_gen);
    StaticGenQueue<ChunkMeshGenData, MAX_QUEUED_MESHES> &queue = m_mesh_gen_queue_high_prio.in_queue.is_empty() ? m_mesh_gen_queue : m_mesh_gen_queue_high_prio;
    bool exists = queue.in_queue.pop(&mesh_data);
    SDL_UnlockMutex(m_lock_mesh_gen);

    /* No mesh to build */
    if(!exists) {
        return false;
    }

    /* Do not generate if already too far TODO: */
    mesh_data->has_been_dropped = !is_chunk_in_range(m_player.get_position_chunk(), mesh_data->chunk_coords, m_owner->get_frame_info().load_radius);

    if(!mesh_data->has_been_dropped) {
        /* Do the mesh generation */
        chunk_mesh_gen(mesh_data);
    }

    SDL_LockMutex(m_lock_mesh_gen);
    queue.finished.push(mesh_data);
    SDL_UnlockMutex(m_lock_mesh_gen);

    return !m_mesh_gen_queue_high_prio.in_queue.is_empty() || !m_mesh_gen_queue.in_queue.is_empty();
}
