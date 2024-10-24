#pragma once

#include "common.h"
#include "shader.h"
#include "texture.h"
#include "chunk.h"

#include "meh_hash.h"

class Game;

inline static uint64_t func_hash_chunk_key(const vec2i &key);
inline static bool func_compare_chunk_key(const vec2i &key_a, const vec2i &key_b);

// @todo Should probably preallocate Chunks or just insert values? new'ing is slow
typedef meh::Table<vec2i, Chunk *, func_hash_chunk_key, func_compare_chunk_key> ChunkHashTable;

class World {
public:
    CLASS_COPY_DISABLE(World);
    
    friend Game;
    friend Chunk;

    /* @temp */
    explicit World(Game *game);
    ~World(void);

    /* Resets world if exists, and sets the new seed */
    void initialize_world(int32_t seed);
    int32_t get_seed(void);

    /* Deletes every chunk from the m_chunks map */
    void delete_chunks(void);

    /* Push chunk to vao loading queue */
    void queue_chunk_vao_load(vec2i chunk_xz);

    /* Generate data for VAO generation on main thread, called from different thread */
    void process_load_queue(void);

    /* Generate VAOs, called on main thread */
    void process_gen_queue(void);

    /* Get chunk or create if doesn't exist */
    Chunk *get_chunk(vec2i chunk_xz, bool create_if_doesnt_exist = false);
    void   gen_chunk(vec2i chunk_xz);

    /* Get block from absolute block position (not relative to a chunk) */
    Block *get_block(const vec3i &block);

private:
    /* Allocates a chunk and generates terrain for given key (Queues the generation of VAO) */
    Chunk *gen_chunk_really(vec2i chunk_xz);

    Game          *m_owner;
    int32_t        m_world_gen_seed;
    ChunkHashTable m_chunk_table;

    /* Chunk VAO load queue */
    std::vector<vec2i> m_load_queue;
    bool m_should_sort_load_queue;
    std::vector<ChunkVaoGenData> m_gen_queue;
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

struct WorldPosition {
    static WorldPosition from_real(const vec3 &real);
    static WorldPosition from_block(const vec3i &block);
    vec3  real;
    vec3i block;
    vec3i block_rel;
    vec2i chunk;
};

/* block's real position is it's origin (0,0,0 corner) */
vec3  real_position_from_block(const vec3i &block);
vec3i block_position_from_real(const vec3 &real);
vec2i chunk_position_from_block(const vec3i &block);
vec3i block_origin_from_chunk(const vec2i &chunk);
vec3i block_relative_from_block(const vec3i &block);
vec3i block_position_from_relative(const vec3i &block_rel, const vec2i &chunk);
void  calc_overlapping_blocks(vec3 pos, vec3 size, WorldPosition &min, WorldPosition &max);

struct RaycastBlockResult {
    bool found;
    vec3 normal;
    vec3 intersection;
    float distance;
    WorldPosition block_p;
};

RaycastBlockResult raycast_block(World &world, const vec3 &ray_origin, const vec3 &ray_end);
bool ray_plane_intersection(const vec3 &ray_origin, const vec3 &ray_end, const vec3 &plane_p, const vec3 &plane_normal, float &out_k, vec3 &out_p);
bool ray_triangle_intersection(const vec3 &ray_origin, const vec3 &ray_end, const vec3 &tri_a, const vec3 &tri_b, const vec3 &tri_c, float &out_k, vec3 &out_p);
