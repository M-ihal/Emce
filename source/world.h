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

    /* */
    Player &get_player(void);

    /* Delete current world and initialize new one */
    void initialize_new_world(uint32_t seed);

    /* Delete chunks and queues */
    void delete_chunks(void);

    /* Delete the chunk */
    void delete_chunk(vec2i chunk_coords);

    /* Return chunk if has been generated */
    Chunk *get_chunk(vec2i chunk_xz);

    /* Create chunk if doesn't exist, and return it */
    Chunk *get_chunk_create(vec2i chunk_xz, BlockType blocks[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z] = NULL);

    /* Return Block and _opt_ Chunk from absolute position, On fail return BlockType::_INVALID */
    BlockType get_block(vec3i block_abs, Chunk **out_chunk = NULL);

    /* Queue chunk to be created */
    void queue_create_chunk(vec2i chunk_xz);

    /* Queue chunks in range from origin to be created */
    void queue_create_chunks(vec2i origin, int32_t radius);

    /* Queue chunk meshes to be build */
    void queue_build_mesh(vec2i chunk_coords);

    /* Build mesh for chunk on main thread */
    void rebuild_mesh_slow(vec2i chunk_coords);

    /* Check if chunk's neighbours are generated */
    bool chunk_neighbours_generated(vec2i chunk_xz);

    /* Get current world state info */
    GameWorldInfo get_world_info(void);

    /* Iterates through only valid chunks */
    bool iterate_chunks(ChunkHashTable::Iterator &iter);

    /*  */
    ChunkMeshGenData *get_next_queue_mesh(void);
    ChunkMeshGenData *get_next_built_mesh(void);
    void push_mesh_queue(ChunkMeshGenData *mesh_data);
    void push_mesh_built(ChunkMeshGenData *mesh_data);

private:

    /* Queue for chunk meshing */
    struct MeshQueue {
        ChunkMeshGenData *queue[MAX_QUEUED_MESHES];
        int32_t front;
        int32_t back;
        int32_t size;

        bool is_full(void);
        bool is_empty(void);
        void push_queue(ChunkMeshGenData *mesh_data);
        ChunkMeshGenData *pop_queue(void);
    };

    Game          *m_owner;
    Player         m_player;
    ChunkHashTable m_chunk_table;
    WorldGenSeed   m_gen_seed;

    SDL_Mutex *m_lock_chunk_gen;
    SDL_Mutex *m_lock_mesh_gen;
    MeshQueue m_meshes_queue;
    MeshQueue m_meshes_built;

    template <typename Type, size_t SIZE>
    struct StaticGenQueue {
        Type    elements[SIZE];
        int32_t elements_free[SIZE];
        int32_t elements_free_count;

        struct Queue {
            Type *queue[SIZE];
            int32_t front;
            int32_t back;
            int32_t size;

            inline bool is_full(void) {
                return size == SIZE;
            }

            inline bool is_empty(void) {
                return size == 0;
            }

            void push(Type value);
            Type pop(void);
        };

        Queue in_queue;
        Queue finished;
    };

#define MAX_QUEUED_CHUNKS 32
    StaticGenQueue<ChunkGenData, MAX_QUEUED_CHUNKS> m_chunk_gen_queue;

    ChunkGenData m_chunk_gen_data[MAX_QUEUED_CHUNKS];
    int32_t m_chunk_gen_data_free[MAX_QUEUED_CHUNKS];
    int32_t m_chunk_gen_data_free_count;

    struct ChunkQueue {
        ChunkGenData *queue[MAX_QUEUED_CHUNKS];
        int32_t front;
        int32_t back;
        int32_t size;
    };

    std::queue<ChunkGenData>  m_chunks_to_generate;
    std::vector<ChunkGenData> m_chunks_generated;
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
