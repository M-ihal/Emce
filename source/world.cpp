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
    for(auto &data : m_chunks_to_generate) { }
    for(auto &data : m_generated_chunks) { }
    m_chunks_to_generate.clear();
    m_generated_chunks.clear();
    for(auto &data : m_meshes_to_build) { chunk_mesh_gen_data_free(data); }
    for(auto &data : m_built_meshes) { chunk_mesh_gen_data_free(data); }
    m_meshes_to_build.clear();
    m_built_meshes.clear();
    SDL_UnlockMutex(m_lock_chunk_gen);
    SDL_UnlockMutex(m_lock_mesh_gen);

    ChunkHashTable::Iterator iter;
    while(m_chunk_table.iterate_all(iter)) {
        Chunk *chunk = iter.value;
        chunk->m_chunk_vao.delete_vao();
        chunk->m_water_vao.delete_vao();
        delete chunk;
    }
    m_chunk_table.clear_table();
}

WorldGenSeed World::get_gen_seed(void) {
    return m_gen_seed;
}

Chunk *World::get_chunk(vec2i chunk_xz) {
    Chunk **chunk_hash = m_chunk_table.find(chunk_xz);
    Chunk *chunk = NULL;
    if(chunk_hash) {
        chunk = *chunk_hash;
    }
    return chunk;
}

Chunk *World::get_chunk_create(vec2i chunk_xz) {
    Chunk **chunk_hash = m_chunk_table.find(chunk_xz);
    Chunk *chunk = NULL;
    if(chunk_hash) {
        chunk = *chunk_hash;
    } else {
        ChunkGenData gen_data;
        chunk_gen_data_init(gen_data, chunk_xz, m_gen_seed);
        chunk_gen(gen_data);

        chunk = new Chunk(this, chunk_xz);
        memcpy(chunk->m_blocks, gen_data.blocks, CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * sizeof(Block));
        m_chunk_table.insert(chunk_xz, chunk);

        SDL_LockMutex(m_lock_mesh_gen);
        ChunkMeshGenData mesh_data;
        chunk_mesh_gen_data_init(mesh_data, *this, chunk_xz);
        m_meshes_to_build.push_back(mesh_data);
        SDL_UnlockMutex(m_lock_mesh_gen);
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

void World::create_chunks_in_range(vec2i origin, int32_t radius) {
    for(int32_t i = 0; i < radius; ++i) {
        for(int32_t j = -i; j <= i; ++j) {
            for(int32_t k = -i; k <= i; ++k) {
                vec2i chunk_xz = origin + vec2i{ j, k };
                this->get_chunk_create(chunk_xz);
            }
        }
    }
}

void World::create_chunk_offload(vec2i chunk_xz) {
    if(!m_chunk_table.contains(chunk_xz)) {
        SDL_LockMutex(m_lock_chunk_gen);

        /* @TODO Stupid check if queued already */
        bool queued = false;
        for(ChunkGenData &gen_data : m_chunks_to_generate) {
            if(gen_data.chunk_xz == chunk_xz) {
                queued = true;
                break;
            }
        }
        SDL_UnlockMutex(m_lock_chunk_gen);

        if(!queued) {
            ChunkGenData gen_data;
            chunk_gen_data_init(gen_data, chunk_xz, get_gen_seed());

            SDL_LockMutex(m_lock_chunk_gen);
            m_chunks_to_generate.push_back(gen_data);
            SDL_UnlockMutex(m_lock_chunk_gen);
        }
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


void World::gen_chunk_vao_imm(vec2i chunk_xz) {
    ChunkMeshGenData gen_data;
    chunk_mesh_gen_data_init(gen_data, *this, chunk_xz);
    chunk_mesh_gen(gen_data);

    Chunk *chunk = this->get_chunk(chunk_xz);
    chunk->set_mesh_vao(gen_data);
}
