#pragma once

#include "common.h"
#include "shader.h"
#include "texture.h"
#include "chunk.h"

#include <unordered_map>

class Game;

typedef std::unordered_map<uint64_t, Chunk *> chunk_map;

class World {
public:
    CLASS_COPY_DISABLE(World);
    
    friend class Game;

    /* @temp */
    explicit World(const Game *game);
    ~World(void);

    /* Resets world if exists, and sets the new seed */
    void initialize_world(int32_t seed);
    int32_t get_seed(void) const;

    /* Deletes every chunk from the m_chunks map */
    void delete_chunks(void);

    /* Push chunk to vao loading queue */
    void queue_chunk_vao_load(vec2i chunk_xz);

    /* Generate data for VAO generation on main thread, called from different thread */
    void process_load_queue(void);

    /* Generate VAOs, called on main thread */
    void process_gen_queue(void);

    /* move out to GameRenderer or something */
    void render_chunks(const Shader &shader, const Texture &atlas);

    /* Get chunk or create if doesn't exist */
    Chunk       *get_chunk(const vec2i &chunk_xz, bool create_if_doesnt_exist = false);
    const Chunk *get_chunk(const vec2i &chunk_xz) const;
    Chunk       *gen_chunk(const vec2i &chunk_xz);

    /* Get block from absolute block position (not relative to a chunk) */
    Block       *get_block(const vec3i &block);
    const Block *get_block(const vec3i &block) const;

    /* Reference to the map containing allocated chunks */
    const chunk_map &get_chunk_map(void);
    const size_t     get_chunk_map_size(void) const;

    /* debug @temp */
    bool _debug_render_not_fill;

private:
    /* Allocates a chunk and generates terrain for given key (Queues the generation of VAO) */
    Chunk *gen_chunk(uint64_t key);

    const Game *m_owner;
    int32_t     m_world_gen_seed;
    chunk_map   m_chunks;

    /* Chunk VAO load queue */
    std::vector<vec2i> m_load_queue;
    bool m_should_sort_load_queue;

    std::vector<ChunkVaoGenData> m_gen_queue;
};

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
vec3i block_relative_from_block(const vec3i &block);
void  calc_overlapping_blocks(vec3 pos, vec3 size, WorldPosition &min, WorldPosition &max);
