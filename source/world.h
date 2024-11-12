#pragma once

#include "common.h"
#include "shader.h"
#include "texture.h"
#include "chunk.h"
#include "chunk_gen.h"
#include "chunk_mesh_gen.h"
#include "meh_hash.h"

#include <SDL3/SDL.h>
#include <vector>

inline static uint64_t func_hash_chunk_key(const vec2i &key);
inline static bool func_compare_chunk_key(const vec2i &key_a, const vec2i &key_b);
typedef meh::Table<vec2i, Chunk *, func_hash_chunk_key, func_compare_chunk_key> ChunkHashTable;


class Game;

class World {
public:
    CLASS_COPY_DISABLE(World);
    
    friend Game;
    friend Chunk;

    explicit World(Game *game);
    ~World(void);

    /* Resets world if exists, and sets the new seed, NEED STOP THREADS BEFORE CALLING */
    void initialize_world(uint32_t seed);

    /* Deletes chunks and gen queues, NEED STOP THREADS BEFORE CALLING */
    void delete_chunks(void);

    /* Current gen seed */
    WorldGenSeed get_gen_seed(void);

    /* Returns chunk if exists */
    Chunk *get_chunk(vec2i chunk_xz);

    /* Immediately creates chunk and returns it, Only call on main thread */
    Chunk *get_chunk_create(vec2i chunk_xz);

    /* Returns Block and _opt_ Chunk from absolute position */
    Block *get_block(vec3i block_abs, Chunk **out_chunk = NULL);

    /* Immediately create chunks, Only call on main thread */
    void create_chunks_in_range(vec2i origin, int32_t radius);

    /* Offload chunk creationto other thread */
    void create_chunk_offload(vec2i chunk_xz);

    /* Offload chunks creation to other thread */
    void create_chunks_in_range_offload(vec2i origin, int32_t radius);

private:
    /* Immediately generate vao for chunk, Only call on main thread */
    void gen_chunk_vao_imm(vec2i chunk_xz);

    /* Pointer to the Game class where world lives */
    Game *m_owner;

    /* Hash table storing the chunks */
    ChunkHashTable m_chunk_table;

    SDL_Mutex *m_lock_chunk_gen;
    SDL_Mutex *m_lock_mesh_gen;

    std::vector<ChunkGenData> m_chunks_to_generate;
    std::vector<ChunkGenData> m_generated_chunks;
    std::vector<ChunkMeshGenData> m_meshes_to_build;
    std::vector<ChunkMeshGenData> m_built_meshes;

    WorldGenSeed m_gen_seed;
};

inline static uint64_t func_hash_chunk_key(const vec2i &key) {
    vec2i x = key;
    x.x += 0;
    x.y += 0;
    x.x *= 11;
    x.y *= 31;
    uint64_t hash = (*(uint64_t *)&x.x) | ((*(uint64_t *)&x.y) << 32);
    return hash;
}

inline static bool func_compare_chunk_key(const vec2i &key_a, const vec2i &key_b) {
    bool equal = key_a.x == key_b.x && key_a.y == key_b.y;
    return equal;
}
