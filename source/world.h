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

struct GameWorldInfo {
    uint32_t world_gen_seed;
    int32_t chunks_loaded;
    int32_t chunks_allocated;
    int32_t chunks_queued;
    int32_t meshes_queued;
};

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

    void delete_chunk(vec2i chunk_coords);

    /* Returns chunk if has been generated */
    Chunk *get_chunk(vec2i chunk_xz);

    /* Creates chunk if doesn't exist, and returns it, ONLY MAIN THREAD */
    Chunk *get_chunk_create(vec2i chunk_xz, BlockType blocks[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z] = NULL);

    /* Returns Block and _opt_ Chunk from absolute position, On fail returns BlockType::_INVALID */
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

    GameWorldInfo get_world_info(void) {
        GameWorldInfo info;
        info.world_gen_seed = m_gen_seed.seed;
        info.chunks_loaded = m_chunk_table.get_count();
        info.chunks_allocated = m_chunk_table.get_size();
        info.chunks_queued = m_chunks_to_generate.size();
        info.meshes_queued = m_meshes_to_build.size();
        return info;
    }


    /* Iterates through only valid chunks */
    bool iterate_chunks(ChunkHashTable::Iterator &iter) {
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
