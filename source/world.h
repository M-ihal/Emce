#pragma once

#include "common.h"
#include "shader.h"
#include "texture.h"
#include "chunk.h"

#include "meh_hash.h"

class  Game;
struct WorldStructure;

inline static uint64_t func_hash_chunk_key(const vec2i &key);
inline static bool func_compare_chunk_key(const vec2i &key_a, const vec2i &key_b);

// @todo Should probably preallocate Chunks or just insert values? new'ing is slow
typedef meh::Table<vec2i, Chunk *, func_hash_chunk_key, func_compare_chunk_key> ChunkHashTable;

class World {
public:
    CLASS_COPY_DISABLE(World);
    
    friend Game;
    friend Chunk;

    static constexpr int32_t ocean_level = 64;

    /* @temp */
    explicit World(Game *game);
    ~World(void);

    /* Resets world if exists, and sets the new seed */
    void initialize_world(int32_t seed);
    int32_t get_seed(void);

    /* Deletes every chunk from the m_chunks map */
    void delete_chunks(void);

    /* Push chunk to vao loading queue, and maybe neighbours if changed_block_rel is on the edge */
    void queue_chunk_vao_load(vec2i chunk_xz, const vec3i *const changed_block_rel = NULL);

    /* Allocate and generate chunks on different thread */
    void process_gen_queue(void);

    /* Generate chunk vao data on different thread */
    void process_vao_queue(void);

    /* Upload all chunk vao queue to the gpu */
    void process_gpu_queue(void);

    /* Get chunk or create if doesn't exist */
    Chunk *get_chunk(vec2i chunk_xz, bool create_if_doesnt_exist = false);

    /* Get chunk from absolute position, and optionally chunk */
    Block *get_block(vec3i block_abs, Chunk **out_chunk = NULL, bool create_if_doesnt_exist = false);

    /* Offload chunk generation to different thread */
    void gen_chunk(vec2i chunk_xz);

    /* Immediately generate chunk */
    void gen_chunk_imm(vec2i chunk_xz);

    /* Queue loading chunks around origin if aren't loaded */
    void gen_chunks(vec2i origin_chunk_xz, uint32_t load_radius);

private:
    /* Allocates a chunk and generates terrain for given key (Queues the generation of VAO) */
    Chunk *gen_chunk_really(vec2i chunk_xz);

    /* Fills given height map array with chunk height values */
    void gen_chunk_height_map(vec2i chunk_xz, int32_t height_map[CHUNK_SIZE_X][CHUNK_SIZE_Z]);

    /* Inserts World structure around given origin, if structure expands out of chunk, generates that chunk */
    void insert_world_structure(const WorldStructure &structure, const vec3i &origin);

    /* Update loaded chunks */
    void update_loaded_chunks(float delta_time);

    Game          *m_owner;
    int32_t        m_world_gen_seed;
    ChunkHashTable m_chunk_table;

    /* Chunk load queue */
    bool m_should_sort_gen_queue;
    bool m_should_sort_vao_queue;
    std::vector<vec2i>           m_gen_queue;
    std::vector<vec2i>           m_vao_queue;
    std::vector<ChunkVaoGenData> m_gpu_queue;
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
vec2i chunk_position_from_real(const vec3 &real);
vec3i block_origin_from_chunk(const vec2i &chunk);
vec3i block_relative_from_block(const vec3i &block);
vec3i block_position_from_relative(const vec3i &block_rel, const vec2i &chunk);
void  calc_overlapping_blocks(vec3 pos, vec3 size, WorldPosition &min, WorldPosition &max);

struct RaycastBlockResult {
    bool found;
    vec3i normal;
    vec3 intersection;
    float distance;
    BlockSide side;
    WorldPosition block_p;
};

RaycastBlockResult raycast_block(World &world, const vec3 &ray_origin, const vec3 &ray_end);
bool ray_plane_intersection(const vec3 &ray_origin, const vec3 &ray_end, const vec3 &plane_p, const vec3 &plane_normal, float &out_k, vec3 &out_p);
bool ray_triangle_intersection(const vec3 &ray_origin, const vec3 &ray_end, const vec3 &tri_a, const vec3 &tri_b, const vec3 &tri_c, float &out_k, vec3 &out_p);

struct WorldStructureBlock {
    BlockType type; /* Type of the block to be inserted */
    vec3i rel_p;    /* Block position relative to origin */
};

struct WorldStructure {
    uint32_t            block_count = 0;
    WorldStructureBlock blocks[128];
    void push_block(vec3i rel_p, BlockType type);
};
