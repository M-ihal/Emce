#include "world.h"

#include "game.h"
#include "debug_ui.h"

#include <algorithm>

#include <glew.h>

#define STB_PERLIN_IMPLEMENTATION
#include <stb_perlin.h>

// @todo Bug with threading process_gen_chunks or something, sometimes slows down...

World::World(Game *game) : m_chunk_table(3000) {
    m_owner = game;
    m_world_gen_seed = 2137;
    m_load_queue.clear();
    m_gen_queue.clear();
    _debug_render_not_fill = false;
}

World::~World(void) {
    this->delete_chunks();
}

void World::initialize_world(int32_t seed) {
    this->delete_chunks();
    m_world_gen_seed = seed;
}

int32_t World::get_seed(void) {
    return m_world_gen_seed;
}

void World::delete_chunks(void) {
    ChunkHashTable::Iterator iter;
    while(m_chunk_table.iterate_all(iter)) {
        Chunk *chunk = *iter.value;
        chunk->m_chunk_vao.delete_vao();
        delete chunk;
    }

    m_chunk_table.clear_table();
    m_load_queue.clear();
}

struct MutexTicket;
extern MutexTicket mutex_load_queue;
extern MutexTicket mutex_gen_queue;
extern void begin_ticket_mutex(MutexTicket &mutex);
extern void end_ticket_mutex(MutexTicket &mutex);

void World::queue_chunk_vao_load(vec2i chunk_xz) {
    begin_ticket_mutex(mutex_load_queue);
    m_load_queue.push_back(chunk_xz);
    end_ticket_mutex(mutex_load_queue);
    m_should_sort_load_queue = true;
}

void World::process_load_queue(void) {
    /* Sorting by distance for now */
    auto sort_func = [&] (const vec2i &c1, const vec2i &c2) -> bool {
        WorldPosition player_chunk_pos = WorldPosition::from_real(m_owner->get_player().get_position());
        vec2i half_chunk_size = vec2i{ 16, 16 } / 2;
        vec2i rel_dist_1 = vec2i::absolute(player_chunk_pos.chunk - c1);
        vec2i rel_dist_2 = vec2i::absolute(player_chunk_pos.chunk - c2);

        // stupid
        float dist_1 = vec2::length(vec2::make(rel_dist_1));
        float dist_2 = vec2::length(vec2::make(rel_dist_2));

        return dist_1 > dist_2; 
    };

    Chunk *chunk = NULL;

    begin_ticket_mutex(mutex_load_queue);
    if(!m_load_queue.empty()) {
        if(m_should_sort_load_queue) {
            std::sort(m_load_queue.begin(), m_load_queue.end(), sort_func);
            m_should_sort_load_queue = false;
        }

        auto end = m_load_queue.back();
        // fprintf(stdout, "Proccessing: %d %d\n", end.x, end.y);
        chunk = this->get_chunk(end);
        m_load_queue.pop_back();
    }
    end_ticket_mutex(mutex_load_queue);

    if(chunk == NULL) {
        return;
    }

    ChunkVaoGenData vao_data = chunk->gen_vao_data();

    begin_ticket_mutex(mutex_gen_queue);
    m_gen_queue.push_back(vao_data);
    end_ticket_mutex(mutex_gen_queue);
}

void World::process_gen_queue(void) {
    begin_ticket_mutex(mutex_gen_queue);
    while(m_gen_queue.size()) {
        ChunkVaoGenData &gen_data = m_gen_queue.back();
        Chunk *chunk = this->get_chunk(gen_data.chunk);
        chunk->gen_vao(gen_data);
        m_gen_queue.pop_back();
    }
    end_ticket_mutex(mutex_gen_queue);
}

void World::render_chunks(const Shader &shader, const Texture &atlas) {
    shader.use_program();
    shader.upload_int("u_texture", 0);
    atlas.bind_texture_unit(0);

    uint32_t num_of_triangles = 0;

    if(_debug_render_not_fill) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    ChunkHashTable::Iterator iter;
    while(m_chunk_table.iterate_all(iter)) {
        Chunk *chunk = *iter.value;

        if(!chunk->m_chunk_vao.has_been_created()) {
            continue;
        }

        const int32_t z = iter.key.y;
        const int32_t x = iter.key.x;

        shader.upload_mat4("u_model", mat4::translate(vec3{ float(x * CHUNK_SIZE_X), 0.0f, float(z * CHUNK_SIZE_Z) }).e);

        chunk->m_chunk_vao.bind_vao();
        GL_CHECK(glDrawElements(GL_TRIANGLES, chunk->m_chunk_vao.get_ibo_count(), GL_UNSIGNED_INT, 0));

        num_of_triangles += chunk->m_chunk_vao.get_ibo_count() / 3;

    }


    _rendered_triangles_last_frame = num_of_triangles;

    if(_debug_render_not_fill) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

extern MutexTicket mutex_get_chunk;

Chunk *World::get_chunk(vec2i chunk_xz, bool create_if_doesnt_exist) {
    begin_ticket_mutex(mutex_get_chunk);
    Chunk **chunk = m_chunk_table.find(chunk_xz);
    end_ticket_mutex(mutex_get_chunk);
    if(chunk != NULL) {
        return *chunk;
    }

    if(!create_if_doesnt_exist) {
        return NULL;
    }

    return this->gen_chunk_really(chunk_xz);
}

void World::gen_chunk(vec2i chunk_xz) {
    Chunk *ignored = this->get_chunk(chunk_xz, true);
}

Block *World::get_block(const vec3i &block) {
    WorldPosition block_p = WorldPosition::from_block(block);

    Chunk *chunk = this->get_chunk(block_p.chunk);
    if(chunk == NULL) {
        /* Chunk doesn't exist */
        return NULL;
    }

    return chunk->get_block(block_p.block_rel);
}

Chunk *World::gen_chunk_really(vec2i chunk_xz) {
    Chunk *created = new Chunk(this, chunk_xz);

    begin_ticket_mutex(mutex_get_chunk);
    m_chunk_table._insert_collisions = 0;
    m_chunk_table.insert(chunk_xz, created);
    end_ticket_mutex(mutex_get_chunk);

    int32_t lowest  = INT32_MAX;
    int32_t highest = INT32_MIN;

    for(int32_t x = 0; x < CHUNK_SIZE_X; ++x) {
        for(int32_t z = 0; z < CHUNK_SIZE_Z; ++z) {
            const float smooth = 0.009f;
            int32_t abs_x = x + CHUNK_SIZE_X * chunk_xz.x;
            int32_t abs_z = z + CHUNK_SIZE_Z * chunk_xz.y;
            float perlin01 = SQUARE((stb_perlin_noise3_seed(abs_x * smooth, 0.0f, abs_z * smooth, 0, 0, 0, m_world_gen_seed) + 1.0f) * 0.5f);
            // float perlin01 = (stb_perlin_noise3_seed(abs_x * smooth, 0.0f, abs_z * smooth, 0, 0, 0, m_world_gen_seed) + 1.0f) * 0.5f;
 //           float perlin01 = 0.5f;
            int32_t height = perlin01 * (CHUNK_SIZE_Y * 0.5f) + CHUNK_SIZE_Y * 0.4f;

            // height = block_position_from_real({ 0.0f, 152.0f, 0.0f }).y - 3;

            if(rand() % 100 == 0) {
                height += 1;
            }

            if(rand() % 1244 == 0) {
                int32_t y_min = height;
                int32_t y_max = y_min + 4;
                if(y_max < CHUNK_SIZE_Y) {
                    for(int32_t y = y_min; y <= y_max; ++y) {
                        Block *block = created->get_block({ x, y, z });
                        block->set_type(BlockType::TREE_LOG);
                    }

                    Block *l1 = created->get_block({x+1, y_max, z}); 
                    if(l1) {
                        l1->set_type(BlockType::TREE_LEAVES);
                    }
                    Block *l2 = created->get_block({x-1, y_max, z}); 
                    if(l2) {
                        l2->set_type(BlockType::TREE_LEAVES);
                    }
                    Block *l3 = created->get_block({x, y_max, z+1}); 
                    if(l3) {
                        l3->set_type(BlockType::TREE_LEAVES);
                    }
                    Block *l4 = created->get_block({x, y_max, z-1}); 
                    if(l4) {
                        l4->set_type(BlockType::TREE_LEAVES);
                    }

                }
            }

            if(lowest > height) lowest = height;
            if(highest < height) highest = height;

            for(int32_t y = 0; y < height; ++y) {
                Block *block = created->get_block({ x, y, z });
                if(y < height / 2) {
                    block->set_type(BlockType::COBBLESTONE);
                } else if(y >= height / 2 && y < CHUNK_SIZE_Y / 2 + 10) {
                    block->set_type(BlockType::SAND);
                } else if(y > CHUNK_SIZE_Y - 40) {
                    block->set_type(BlockType::COBBLESTONE);
                } else if(y < (height - 1)) {
                    block->set_type(BlockType::DIRT);
                } else {
                    block->set_type(BlockType::DIRT);
                    BlockInfo &info = block->get_info();
                    if(rand() % 2) {
                        info.dirt.has_grass = true;
                    }
                }
            }
        }
    }


    this->queue_chunk_vao_load(chunk_xz);

    return created;
}

vec3 real_position_from_block(const vec3i &block) {
    return {
        (float)block.x,
            (float)block.y,
            (float)block.z
    };
}

vec3i block_position_from_real(const vec3 &real) {
    return {
        (int32_t)floorf(real.x),
        (int32_t)floorf(real.y),
        (int32_t)floorf(real.z),
    };
}

vec2i chunk_position_from_block(const vec3i &block) {
    return {
        (int32_t)floorf(float(block.x) / CHUNK_SIZE_X),
        (int32_t)floorf(float(block.z) / CHUNK_SIZE_Z)
    };
}

vec3i block_relative_from_block(const vec3i &block) {
    return { 
        .x = block.x >= 0 ? block.x % CHUNK_SIZE_X : (block.x + 1) % CHUNK_SIZE_X + CHUNK_SIZE_X - 1,
        .y = block.y,
        .z = block.z >= 0 ? block.z % CHUNK_SIZE_Z : (block.z + 1) % CHUNK_SIZE_Z + CHUNK_SIZE_Z - 1,
    };
}

// @todo
vec3i block_position_from_relative(const vec3i &block_rel, const vec2i &chunk) {
    return vec3i{
        .x = chunk.x >= 0 ? chunk.x * CHUNK_SIZE_X + block_rel.x : chunk.x * CHUNK_SIZE_X + block_rel.x,
        .y = block_rel.y,
        .z = chunk.y >= 0 ? chunk.y * CHUNK_SIZE_Z + block_rel.z : chunk.y * CHUNK_SIZE_Z + block_rel.z,
    };
}

void calc_overlapping_blocks(vec3 pos, vec3 size, WorldPosition &min, WorldPosition &max) {
    vec3i block_min_p = block_position_from_real(pos);
    vec3i block_max_p = block_position_from_real(pos + size);
    min = WorldPosition::from_block(block_min_p);
    max = WorldPosition::from_block(block_max_p);
}

WorldPosition WorldPosition::from_real(const vec3 &real) {
    WorldPosition result;
    result.real = real;
    result.block = block_position_from_real(real);
    result.block_rel = block_relative_from_block(result.block);
    result.chunk = chunk_position_from_block(result.block);
    return result;
}

WorldPosition WorldPosition::from_block(const vec3i &block) {
    WorldPosition result;
    result.block = block;
    result.block_rel = block_relative_from_block(block);
    result.chunk = chunk_position_from_block(result.block);
    result.real = real_position_from_block(block);
    return result;
}
