#pragma once

#include "common.h"
#include "shader.h"
#include "texture.h"
#include "chunk.h"
#include "chunk_gen.h"
#include "chunk_mesh_gen.h"
#include "meh_hash.h"
#include "player.h"

#include <SDL3/SDL.h>
#include <vector>
#include <queue>

inline uint64_t func_hash_chunk_key(const vec2i &key);
inline bool     func_compare_chunk_key(const vec2i &key_a, const vec2i &key_b);
typedef meh::Table<vec2i, Chunk *, func_hash_chunk_key, func_compare_chunk_key, 85> ChunkHashTable;

class World {
public:
    CLASS_COPY_DISABLE(World);
    
    friend class Game;

    explicit World(Game *game);
    ~World(void);

    /* Get _Game_ containing that world */
    Game *get_game(void);

    /* Get the player reference */
    Player &get_player(void);

    /* Deletes current world and initializes new one */
    void initialize_new_world(uint32_t seed);

    /* Deletes chunks and gen queues, NEED STOP THREADS BEFORE CALLING */
    void delete_chunks(void);

    /* Update chunks */
    void update_loaded_chunks(float delta_time);

    /* Current gen seed */
    WorldGenSeed get_gen_seed(void);

    /* Access chunk hash table */
    ChunkHashTable &get_chunk_table(void);

    /* Returns chunk if has been generated */
    Chunk *get_chunk(vec2i chunk_xz);

    /* Creates chunk if doesn't exist, and returns it, ONLY MAIN THREAD */
    Chunk *get_chunk_create(vec2i chunk_xz, BlockType blocks[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z] = NULL);

    /* Returns Block and _opt_ Chunk from absolute position */
    BlockType get_block(vec3i block_abs, Chunk **out_chunk = NULL);

    /* Load chunks in range, ONLY MAIN THREAD */
    void create_chunks_in_range(vec2i origin, int32_t radius);

    /* Offload chunk creati onto other thread */
    void create_chunk_offload(vec2i chunk_xz);

    /* Offload chunks creation to other thread */
    void create_chunks_in_range_offload(vec2i origin, int32_t radius);

    /* Re/Generate chunk vao immediatelly, Only main thread */
    void gen_chunk_mesh_imm(vec2i chunk_xz);

    /* Offload chunk mesh generation */
    void gen_chunk_mesh_offload(vec2i chunk_xz);

    /* Checks if chunk's neighbours are generated */
    bool chunk_neighbours_generated(vec2i chunk_xz);

private:
    Game *m_owner;

    Player         m_player;
    ChunkHashTable m_chunk_table;
    WorldGenSeed   m_gen_seed;

    SDL_Mutex *m_lock_chunk_gen;
    SDL_Mutex *m_lock_mesh_gen;

    std::queue<ChunkGenData>  m_chunks_to_generate;
    std::vector<ChunkGenData> m_chunks_generated;
    std::queue<ChunkMeshGenData *>  m_meshes_to_build;
    std::vector<ChunkMeshGenData *> m_meshes_built;
};

inline uint64_t func_hash_chunk_key(const vec2i &key) {
    vec2i x = key;
    x.x += 0;
    x.y += 0;
    x.x *= 11;
    x.y *= 31;
    uint64_t hash = (*(uint64_t *)&x.x) | ((*(uint64_t *)&x.y) << 32);
    return hash;
}

inline bool func_compare_chunk_key(const vec2i &key_a, const vec2i &key_b) {
    bool equal = key_a.x == key_b.x && key_a.y == key_b.y;
    return equal;
}
