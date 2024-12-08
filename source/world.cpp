#include "world.h"
#include "game.h"
#include "chunk_gen.h"

// @temp
#include "window.h"

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

    /* Queue loading chunks */
    /// m_owner->check_for_chunks_to_load();
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

    while(!m_meshes_to_build.empty()) {
        ChunkMeshGenData *mesh_data = m_meshes_to_build.front();
        m_meshes_to_build.pop();
        chunk_mesh_gen_data_free(&mesh_data);
    }
    m_meshes_to_build = std::queue<ChunkMeshGenData *>();

    for(ChunkMeshGenData *mesh_data : m_meshes_built) {
        chunk_mesh_gen_data_free(&mesh_data);
    }
    m_meshes_built.clear();

    ChunkHashTable::Iterator iter;
    while(m_chunk_table.iterate_all(iter)) {
        Chunk *chunk = iter.value;
        chunk->delete_vaos();
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
        if((*chunk_hash)->m_state == ChunkState::GENERATING) {
            return NULL;
        } else {
            return *chunk_hash;
        }
    }
    return NULL;
}

Chunk *World::get_chunk_create(vec2i chunk_xz, BlockType blocks[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z]) {
    Chunk *to_generate = NULL;
    Chunk **chunk_hash = m_chunk_table.find(chunk_xz);
    if(chunk_hash != NULL) {
        Chunk *chunk = *chunk_hash;
        if(chunk->m_state == ChunkState::GENERATED) {
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
            to_generate->set_chunk_blocks(blocks);
        } else {
            ChunkGenData gen_data;
            chunk_gen_data_init(gen_data, chunk_xz, m_gen_seed);
            chunk_gen(gen_data);

            to_generate->set_chunk_blocks(gen_data.blocks);
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

void World::create_chunks_in_range(vec2i origin, int32_t radius) {
    for(int32_t i = 0; i < radius; ++i) {
        for(int32_t j = -i; j <= i; ++j) {
            for(int32_t k = -i; k <= i; ++k) {
                vec2i chunk_xz = origin + vec2i{ j, k };
                get_chunk_create(chunk_xz);
            }
        }
    }
}

void World::create_chunk_offload(vec2i chunk_xz) {
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

void World::create_chunks_in_range_offload(vec2i origin, int32_t radius) {
    for(int32_t i = 0; i < radius; ++i) {
        for(int32_t j = -i; j <= i; ++j) {
            for(int32_t k = -i; k <= i; ++k) {
                vec2i chunk_xz = origin + vec2i{ j, k };
                this->create_chunk_offload(chunk_xz);
            }
        }
    }
}

void World::gen_chunk_mesh_offload(vec2i chunk_xz) {
    Chunk *chunk = this->get_chunk(chunk_xz);
    if(chunk == NULL) {
        /* The chunk isn't loaded ... */
        return;
    }

    SDL_LockMutex(m_lock_mesh_gen);
    ChunkMeshGenData *mesh_data = NULL;
    chunk_mesh_gen_data_init(&mesh_data, *this, chunk_xz);
    if(mesh_data != NULL) {
        m_meshes_to_build.push(mesh_data);
    }
    SDL_UnlockMutex(m_lock_mesh_gen);

    if(mesh_data != NULL) {
        chunk->m_mesh_state = ChunkMeshState::QUEUED;
        m_owner->wake_up_gen_meshes_threads();
    }
}

void World::gen_chunk_mesh_imm(vec2i chunk_xz) {
    Chunk *chunk = this->get_chunk(chunk_xz);
    if(chunk == NULL) {
        /* The chunk isn't loaded ... */
        return;
    }

    ChunkMeshGenData *mesh_data = new ChunkMeshGenData;

    chunk_mesh_gen_data_init(&mesh_data, *this, chunk_xz, true);
    chunk_mesh_gen(mesh_data);
    chunk->set_mesh(mesh_data);
    chunk->m_mesh_state = ChunkMeshState::LOADED;
    chunk_mesh_gen_data_free(&mesh_data);

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
