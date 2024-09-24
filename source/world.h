#pragma once

#include "common.h"
#include "shader.h"
#include "texture.h"
#include "chunk.h"

#include <unordered_map>

typedef std::unordered_map<uint64_t, Chunk *> chunk_map;

class World {
public:
    CLASS_COPY_DISABLE(World);

    World(void);
    ~World(void);

    /* Resets world if exists, and sets the new seed */
    void initialize_world(int32_t seed);
    int32_t get_seed(void) const;

    /* Deletes every chunk from the m_chunks map */
    void delete_chunks(void);

    /* */
    void render_chunks(const Shader &shader, const Texture &atlas);

    /* Get chunk or create if doesn't exist */
    Chunk       *get_chunk(const vec2i &chunk_xz, bool create_if_doesnt_exist = false);
    const Chunk *get_chunk(const vec2i &chunk_xz) const;
    Chunk       *gen_chunk(const vec2i &chunk_xz);

    /* Reference to the map containing allocated chunks */
    const chunk_map &get_chunk_map(void);
    const size_t     get_chunk_map_size(void) const;

    /* debug @temp */
    bool _debug_render_not_fill;

private:
    Chunk *generate_chunk(uint64_t packed_xz);

    int32_t   m_world_gen_seed;
    chunk_map m_chunks;
};
