#include "world.h"
#include "game.h"
#include "chunk_gen.h"

#include <algorithm>
#include <glew.h>

World::World(Game *game) {
    m_owner = game;

    m_lock_chunk_gen = SDL_CreateMutex();
    m_lock_mesh_gen = SDL_CreateMutex();

    m_chunk_table.initialize_table(5000);
    this->initialize_world(2137);
}

World::~World(void) {
    this->delete_chunks();

    SDL_DestroyMutex(m_lock_chunk_gen);
    SDL_DestroyMutex(m_lock_mesh_gen);
}

void World::initialize_world(uint32_t seed) {
    this->delete_chunks();
    m_gen_seed.seed = seed;
}

void World::delete_chunks(void) {
    /* Wait for mutexes and delete queues */
    SDL_LockMutex(m_lock_chunk_gen);
    SDL_LockMutex(m_lock_mesh_gen);
    for(auto &data : m_chunks_generated) { }
    m_chunks_generated.clear();

    while(!m_chunks_to_generate.empty()) {
        ChunkGenData gen_data = m_chunks_to_generate.front();
        // chunk_gen_data_free(gen_data);
        m_chunks_to_generate.pop();
    }

    std::queue<ChunkGenData> _queue_empty;
    m_chunks_to_generate = _queue_empty;

    while(!m_meshes_to_build.empty()) {
        ChunkMeshGenData *mesh_data = m_meshes_to_build.front();
        chunk_mesh_gen_data_free(&mesh_data);
        m_meshes_to_build.pop();
    }

    while(!m_meshes_built.empty()) {
        ChunkMeshGenData *mesh_data = m_meshes_built.front();
        chunk_mesh_gen_data_free(&mesh_data);
        m_meshes_built.pop_back();
    }
    m_meshes_built.clear();

    /* Just to make sure */
    std::queue<ChunkMeshGenData *> queue_empty;
    m_meshes_to_build = queue_empty;

    SDL_UnlockMutex(m_lock_chunk_gen);
    SDL_UnlockMutex(m_lock_mesh_gen);

    ChunkHashTable::Iterator iter;
    while(m_chunk_table.iterate_all(iter)) {
        Chunk *chunk = iter.value;
        chunk->delete_vaos();
        delete chunk;
    }
    m_chunk_table.clear_table();
}

void World::update_loaded_chunks(float delta_time) {
    std::vector<vec2i> chunks_to_delete;

    ChunkHashTable::Iterator chunk_iter;
    while(m_chunk_table.iterate_all(chunk_iter)) {
        Chunk *chunk = chunk_iter.value;

        /* If is being generated -> do not update it */
        if(chunk->m_state == ChunkState::GENERATING) {
            continue;
        }

        if(!is_chunk_in_range(chunk_iter.key, m_owner->get_player().get_position_chunk(), get_load_radius())) {
            chunks_to_delete.push_back(chunk_iter.key);
            continue;
        }

        /* If waiting and neighbours got generated -> queue for meshing */
        if(chunk->m_mesh_state == ChunkMeshState::WAITING) {
            if(this->chunk_neighbours_generated(chunk->m_chunk_xz)) {
                this->gen_chunk_mesh_offload(chunk->m_chunk_xz);
            }
        }
    }

    for(vec2i &chunk_xz : chunks_to_delete) {
        Chunk **chunk_hash = m_chunk_table.find(chunk_xz);
        ASSERT(chunk_hash);
        Chunk *chunk = *chunk_hash;
        delete chunk;
        m_chunk_table.remove(chunk_xz);
    }
}

WorldGenSeed World::get_gen_seed(void) {
    return m_gen_seed;
}

ChunkHashTable *World::get_chunk_table(void) {
    return &m_chunk_table;
}

Chunk *World::get_chunk(vec2i chunk_xz) {
    Chunk **chunk_hash = m_chunk_table.find(chunk_xz);
    Chunk *chunk = NULL;
    if(chunk_hash) {
        if((*chunk_hash)->m_state != ChunkState::GENERATING) {
            chunk = *chunk_hash;
        }
    }
    return chunk;
}

Block *World::get_block(vec3i block_abs, Chunk **out_chunk) {
    WorldPosition block_p = WorldPosition::from_block(block_abs);

    Block *block = NULL;
    Chunk *chunk = this->get_chunk(block_p.chunk);
    if(chunk) {
        block = chunk->get_block(block_p.block_rel);
        if(out_chunk) {
            *out_chunk = chunk;
        }
    }
    return block;
}

void World::create_chunk_offload(vec2i chunk_xz) {
    Chunk **chunk_hash = m_chunk_table.find(chunk_xz);
    if(chunk_hash == NULL) {
        Chunk *chunk = new Chunk(this, chunk_xz);
        m_chunk_table.insert(chunk_xz, chunk);

        ChunkGenData gen_data;
        chunk_gen_data_init(gen_data, chunk_xz, get_gen_seed());

        SDL_LockMutex(m_lock_chunk_gen);
        m_chunks_to_generate.push(gen_data);
        SDL_UnlockMutex(m_lock_chunk_gen);
    }
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

    if(chunk->m_mesh_state == ChunkMeshState::QUEUED) {
        /* In the loading queue so do not queue again */
        return;
    }

    SDL_LockMutex(m_lock_mesh_gen);
    ChunkMeshGenData *mesh_data;
    chunk_mesh_gen_data_init(&mesh_data, *this, chunk_xz);
    m_meshes_to_build.push(mesh_data);
    chunk->m_mesh_state = ChunkMeshState::QUEUED;
    SDL_UnlockMutex(m_lock_mesh_gen);
}

void World::gen_chunk_mesh_imm(vec2i chunk_xz) {
    ChunkMeshGenData *mesh_data;
    chunk_mesh_gen_data_init(&mesh_data, *this, chunk_xz);
    chunk_mesh_gen(mesh_data);
    Chunk *chunk = this->get_chunk(mesh_data->chunk_xz);
    ASSERT(chunk);
    chunk->set_mesh_vao(mesh_data);
    chunk_mesh_gen_data_free(&mesh_data);
    chunk->m_mesh_state = ChunkMeshState::LOADED;
}

bool World::chunk_neighbours_generated(vec2i chunk_xz) {
    Chunk *x_neg = this->get_chunk(chunk_xz + vec2i{ -1,  0 });
    Chunk *x_pos = this->get_chunk(chunk_xz + vec2i{ +1,  0 });
    Chunk *z_neg = this->get_chunk(chunk_xz + vec2i{  0, -1 });
    Chunk *z_pos = this->get_chunk(chunk_xz + vec2i{  0, +1 });
    
    return x_neg && x_pos && z_neg && z_pos;
}
