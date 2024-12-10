#include "world.h"
#include "game.h"
#include "chunk_gen.h"

#include <algorithm>
#include <glew.h>

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

    m_meshes_queue = { };
    m_meshes_built = { };
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
    while(!m_chunks_to_generate.empty()) {
        ChunkGenData gen_data = m_chunks_to_generate.front();
        // gen_data_free();
        m_chunks_to_generate.pop();
    }
    m_chunks_to_generate = std::queue<ChunkGenData>();

    for(auto &gen_data : m_chunks_generated) {
        // gen_data_free();
    }
    m_chunks_generated.clear();


    ChunkMeshGenData *queued_data;
    while((queued_data = m_meshes_queue.pop_queue()) != NULL) {
        chunk_mesh_gen_data_free(&queued_data);
    }
    m_meshes_queue = { };

    ChunkMeshGenData *built_data;
    while((built_data = m_meshes_built.pop_queue()) != NULL) {
        chunk_mesh_gen_data_free(&built_data);
    }
    m_meshes_built = { };

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
        to_generate->set_mesh_state(ChunkMeshState::WAITING);
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

    Chunk *chunk = new Chunk(this, chunk_xz);
    m_chunk_table.insert(chunk_xz, chunk);

    ChunkGenData gen_data;
    chunk_gen_data_init(gen_data, chunk_xz, m_gen_seed);

    SDL_LockMutex(m_lock_chunk_gen);
    m_chunks_to_generate.push(gen_data);
    SDL_UnlockMutex(m_lock_chunk_gen);

    m_owner->wake_up_gen_chunks_threads();
}

void World::queue_create_chunks(vec2i origin, int32_t radius) {
    for(int32_t i = 0; i < radius; ++i) {
        for(int32_t j = -i; j <= i; ++j) {
            for(int32_t k = -i; k <= i; ++k) {
                vec2i chunk_xz = origin + vec2i{ j, k };
                this->queue_create_chunk(chunk_xz);
            }
        }
    }
}

void World::queue_build_mesh(vec2i chunk_coords) {
    Chunk *chunk = this->get_chunk(chunk_coords);
    if(chunk == NULL) {
        /* The chunk isn't loaded ... */
        return;
    }

    ChunkMeshGenData *mesh_data = NULL;
    chunk_mesh_gen_data_init(&mesh_data, *this, chunk_coords);

    if(mesh_data) {
        SDL_LockMutex(m_lock_mesh_gen);
        this->push_mesh_queue(mesh_data);
        SDL_UnlockMutex(m_lock_mesh_gen);
    }

    if(mesh_data != NULL) {
        chunk->set_mesh_state(ChunkMeshState::QUEUED);
        m_owner->wake_up_gen_meshes_threads();
    }
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
    chunk_mesh_gen_data_init(&mesh_data, *this, chunk_coords, true);

    chunk_mesh_gen(mesh_data);
    chunk->set_mesh(mesh_data);
    chunk->set_mesh_state(ChunkMeshState::LOADED);

    delete mesh_data;
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
    info.chunks_queued = m_chunks_to_generate.size();
    info.meshes_queued = m_meshes_queue.size;
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

ChunkMeshGenData *World::get_next_queue_mesh(void) {
    return m_meshes_queue.pop_queue();
}

ChunkMeshGenData *World::get_next_built_mesh(void) {
    return m_meshes_built.pop_queue();
}

void World::push_mesh_queue(ChunkMeshGenData *mesh_data) {
    m_meshes_queue.push_queue(mesh_data);
}

void World::push_mesh_built(ChunkMeshGenData *mesh_data) {
    m_meshes_built.push_queue(mesh_data);
}

bool World::MeshQueue::is_full(void) {
    return size >= MAX_QUEUED_MESHES;
}

bool World::MeshQueue::is_empty(void) {
    return size == 0;
}

void World::MeshQueue::push_queue(ChunkMeshGenData *mesh_data) {
    if(this->is_full()) {
        INVALID_CODE_PATH;
        return;
    }

    queue[front] = mesh_data;
    front = (front + 1) % MAX_QUEUED_MESHES;
    size += 1;
}

ChunkMeshGenData *World::MeshQueue::pop_queue(void) {
    if(this->is_empty()) {
        return NULL;
    }

    ChunkMeshGenData *mesh_data = queue[back];
    back = (back + 1) % MAX_QUEUED_MESHES;
    size -= 1;
    return mesh_data;
}

