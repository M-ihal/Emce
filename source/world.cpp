#include "world.h"

#include "game.h"

#include <algorithm>
#include <glew.h>

#define STB_PERLIN_IMPLEMENTATION
#include <stb_perlin.h>

#include <PerlinNoise.hpp>

struct MutexTicket;
extern MutexTicket mutex_chunk_gen_queue;
extern MutexTicket mutex_chunk_vao_queue;
extern MutexTicket mutex_chunk_gpu_queue;
extern MutexTicket mutex_world_get_chunk;
extern void begin_ticket_mutex(MutexTicket &mutex);
extern void end_ticket_mutex(MutexTicket &mutex);

World::World(Game *game) {
    m_owner = game;
    m_world_gen_seed = 2137;
    m_gen_queue.clear();
    m_vao_queue.clear();
    m_gpu_queue.clear();
    m_chunk_table.initialize_table(3000);
}

World::~World(void) {
    this->delete_chunks();
    m_chunk_table.delete_table();
}

void World::initialize_world(int32_t seed) {
    this->delete_chunks();
    m_world_gen_seed = seed;
}

int32_t World::get_seed(void) {
    return m_world_gen_seed;
}

void World::delete_chunks(void) {
    begin_ticket_mutex(mutex_chunk_gen_queue);
    begin_ticket_mutex(mutex_chunk_vao_queue);
    begin_ticket_mutex(mutex_chunk_gpu_queue);

    ChunkHashTable::Iterator iter;
    while(m_chunk_table.iterate_all(iter)) {
        Chunk *chunk = iter.value;
        chunk->m_chunk_vao.delete_vao();
        chunk->m_water_vao.delete_vao();
        delete chunk;
    }

    m_chunk_table.clear_table();
    m_vao_queue.clear();
    m_gpu_queue.clear();

    end_ticket_mutex(mutex_chunk_gpu_queue);
    end_ticket_mutex(mutex_chunk_vao_queue);
    end_ticket_mutex(mutex_chunk_gen_queue);
}

void World::queue_chunk_vao_load(vec2i chunk_xz, const vec3i *const changed_block_rel) {
    if(!m_chunk_table.contains(chunk_xz)) {
        return;
    }

    begin_ticket_mutex(mutex_chunk_vao_queue);
    
    // @todo STUPID
    // #
    bool exists = false;
    for(auto &e : m_vao_queue) {
        if(e == chunk_xz) {
            exists = true;
            break;
        }
    }
    if(!exists) {

        if(chunk_xz.y < -1000) {
            int a = 44;
        }
 //      fprintf(stdout, "Adding to chunk vao load: %d, %d\n", chunk_xz.x, chunk_xz.y);

        m_vao_queue.push_back(chunk_xz);
    }

    end_ticket_mutex(mutex_chunk_vao_queue);

    if(changed_block_rel != NULL) {
        if(is_block_on_x_neg_edge(*changed_block_rel)) {
            this->queue_chunk_vao_load(chunk_xz + vec2i{ -1, 0 });
        } else if(is_block_on_x_pos_edge(*changed_block_rel)) {
            this->queue_chunk_vao_load(chunk_xz + vec2i{ 1, 0 });
        }

        if(is_block_on_z_neg_edge(*changed_block_rel)) {
            this->queue_chunk_vao_load(chunk_xz + vec2i{ 0, -1 });
        } else if(is_block_on_z_pos_edge(*changed_block_rel)) {
            this->queue_chunk_vao_load(chunk_xz + vec2i{ 0, 1 });
        }
    }

    // m_should_sort_vao_queue = true;
}

static float chunk_distance(vec2i chunk_a, vec2i chunk_b) {
    const vec2i half_chunk_size = vec2i{ 16, 16 } / 2;
    const vec2i rel_dist = vec2i::absolute(chunk_a - chunk_b);
    const float dist = vec2::length(vec2::make(rel_dist));
    return dist;
}

void World::process_gen_queue(void) {
    /* Sorting by distance for now */
    auto sort_func = [&] (const vec2i &c1, const vec2i &c2) -> bool {
        const vec2i player_c = m_owner->get_player().get_position_chunk();
        float dist_1 = chunk_distance(player_c, c1);
        float dist_2 = chunk_distance(player_c, c2);
        return dist_1 > dist_2; 
    };

    begin_ticket_mutex(mutex_chunk_gen_queue);
    bool available = false;
    vec2i chunk_xz;
    if(m_gen_queue.size()) {
        if(m_should_sort_gen_queue) {
            std::sort(m_gen_queue.begin(), m_gen_queue.end(), sort_func);
            m_should_sort_gen_queue = false;
        }

        chunk_xz = m_gen_queue.back();
        m_gen_queue.pop_back();
        available = true;
    }
    end_ticket_mutex(mutex_chunk_gen_queue);

    if(available) {
        Chunk *chunk = this->gen_chunk_really(chunk_xz);
    }
}

void World::process_vao_queue(void) {
    /* Sorting by distance for now */
    auto sort_func = [&] (const vec2i &c1, const vec2i &c2) -> bool {
        const vec2i player_c = m_owner->get_player().get_position_chunk();
        float dist_1 = chunk_distance(player_c, c1);
        float dist_2 = chunk_distance(player_c, c2);
        return dist_1 > dist_2; 
    };

    Chunk *chunk = NULL;

    begin_ticket_mutex(mutex_chunk_vao_queue);
    if(!m_vao_queue.empty()) {
        if(m_should_sort_vao_queue) {
            std::sort(m_vao_queue.begin(), m_vao_queue.end(), sort_func);
            m_should_sort_vao_queue = false;
        }

        auto end = m_vao_queue.back();
        chunk = this->get_chunk(end);
        m_vao_queue.pop_back();
    }
    end_ticket_mutex(mutex_chunk_vao_queue);

    if(chunk == NULL) {
        return;
    }

    ChunkVaoGenData vao_data = chunk->gen_vao_data();
    begin_ticket_mutex(mutex_chunk_gpu_queue);
    m_gpu_queue.push_back(std::move(vao_data));
    end_ticket_mutex(mutex_chunk_gpu_queue);
}

void World::process_gpu_queue(void) {
    begin_ticket_mutex(mutex_chunk_gpu_queue);
    while(m_gpu_queue.size()) {
        ChunkVaoGenData gen_data = m_gpu_queue.back();
        Chunk *chunk = this->get_chunk(gen_data.chunk);
        chunk->gen_vao(gen_data);
        free_chunk_vao_gen_data(gen_data);
        m_gpu_queue.pop_back();
    }
    end_ticket_mutex(mutex_chunk_gpu_queue);
}

Chunk *World::get_chunk(vec2i chunk_xz, bool create_if_doesnt_exist) {
   //  begin_ticket_mutex(mutex_world_get_chunk);
    Chunk **chunk_slot = m_chunk_table.find(chunk_xz);
   // end_ticket_mutex(mutex_world_get_chunk);

    Chunk *chunk = NULL;
    if(chunk_slot) {
        chunk = *chunk_slot;
    } else if(create_if_doesnt_exist) {
        chunk = this->gen_chunk_really(chunk_xz);
    }
    return chunk;
}

Block *World::get_block(vec3i block_abs, Chunk **out_chunk, bool create_if_doesnt_exist) {
    WorldPosition block_p = WorldPosition::from_block(block_abs);

    Block *block = NULL;
    Chunk *chunk = this->get_chunk(block_p.chunk, create_if_doesnt_exist);
    if(chunk) {
        block = chunk->get_block(block_p.block_rel);
    }
    if(out_chunk) {
        *out_chunk = chunk;
    }
    return block;
}

void World::gen_chunk(vec2i chunk_xz) {
    if(m_chunk_table.contains(chunk_xz)) {
        return;
    }

    begin_ticket_mutex(mutex_chunk_gen_queue);
    // @todo STUPID
    bool exists = false;
    for(auto &e : m_gen_queue) {
        if(e == chunk_xz) {
            exists = true;
            break;
        }
    }
    if(!exists) {
 //       fprintf(stdout, "Adding to chunk gen queue: %d, %d\n", chunk_xz.x, chunk_xz.y);
        m_gen_queue.push_back(chunk_xz);
        //m_should_sort_gen_queue = true;
    }
    end_ticket_mutex(mutex_chunk_gen_queue);
}

void World::gen_chunk_imm(vec2i chunk_xz) {
    Chunk *ignored = this->get_chunk(chunk_xz, true);
}

void World::gen_chunks(vec2i origin_chunk_xz, uint32_t load_radius) {
    for(int32_t i = 0; i < load_radius; ++i) {
        for(int32_t load_x = -i; load_x <= i; ++load_x) {
            this->gen_chunk(origin_chunk_xz + vec2i{ load_x, +i });
            this->gen_chunk(origin_chunk_xz + vec2i{ load_x, -i });
        }
        for(int32_t load_z = -i + 1; load_z <= (i - 1); ++load_z) {
            this->gen_chunk(origin_chunk_xz + vec2i{ +i, load_z });
            this->gen_chunk(origin_chunk_xz + vec2i{ -i, load_z });
        }
    }
}

void World::gen_chunk_height_map(vec2i chunk_xz, int32_t height_map[CHUNK_SIZE_X][CHUNK_SIZE_Z]) {
    siv::PerlinNoise perlin(m_world_gen_seed);

    const int32_t octaves = 4;
    const double freq_x   = 0.00315;
    const double freq_z   = 0.00315;

    for(int32_t x = 0; x < CHUNK_SIZE_X; ++x) {
        for(int32_t z = 0; z < CHUNK_SIZE_Z; ++z) {

            int32_t abs_x = x + CHUNK_SIZE_X * chunk_xz.x;
            int32_t abs_z = z + CHUNK_SIZE_Z * chunk_xz.y;

            double noise = perlin.octave2D_01(abs_x * freq_x, abs_z * freq_z, octaves);

            const int32_t min_height = 32;

            int32_t height = 0;

            struct Segment { double left; double right; int32_t height_left; int32_t height_right; };

            Segment segments[] = {
                { 0.00, 0.25, 0,   16  },
                { 0.25, 0.30, 16,  32  },
                { 0.30, 0.65, 32,  80  },
                { 0.65, 0.68, 80,  96 },
                { 0.68, 0.75, 96,  128 },
                { 0.75, 0.80, 128, 164 },
                { 0.80, 0.90, 164, 196 },
                { 0.90, 1.00, 196, 200 },
            };

            for(int32_t index = 0; index < ARRAY_COUNT(segments); ++index) {
                Segment seg = segments[index];
                if(noise <= seg.right) {
                    double perc = (noise - seg.left) / (seg.right - seg.left);
                    height = min_height + lerp(seg.height_left, seg.height_right, perc);
                    break;
                }
            }

            height_map[x][z] = height;

        }
    }

}

Chunk *World::gen_chunk_really(vec2i chunk_xz) {
    if(m_chunk_table.contains(chunk_xz)) {
        return NULL;
    }

    Chunk *created = new Chunk(this, chunk_xz);

    WorldStructure struct_tree;
    struct_tree.push_block({ 0, 0, 0 }, BlockType::TREE_LOG);
    struct_tree.push_block({ 0, 1, 0 }, BlockType::TREE_LOG);
    struct_tree.push_block({ 0, 2, 0 }, BlockType::TREE_LOG);
    struct_tree.push_block({ 0, 3, 0 }, BlockType::TREE_LOG);
    struct_tree.push_block({ -1, 2, -1 }, BlockType::TREE_LEAVES);
    struct_tree.push_block({ +1, 2, -1 }, BlockType::TREE_LEAVES);
    struct_tree.push_block({ +1, 2, +1 }, BlockType::TREE_LEAVES);
    struct_tree.push_block({ -1, 2, +1 }, BlockType::TREE_LEAVES);
    struct_tree.push_block({  0, 2, -1 }, BlockType::TREE_LEAVES);
    struct_tree.push_block({  0, 2, +1 }, BlockType::TREE_LEAVES);
    struct_tree.push_block({ +1, 2,  0 }, BlockType::TREE_LEAVES);
    struct_tree.push_block({ -1, 2,  0 }, BlockType::TREE_LEAVES);
    struct_tree.push_block({ -1, 3, -1 }, BlockType::TREE_LEAVES);
    struct_tree.push_block({ +1, 3, -1 }, BlockType::TREE_LEAVES);
    struct_tree.push_block({ +1, 3, +1 }, BlockType::TREE_LEAVES);
    struct_tree.push_block({ -1, 3, +1 }, BlockType::TREE_LEAVES);
    struct_tree.push_block({  0, 3, -1 }, BlockType::TREE_LEAVES);
    struct_tree.push_block({  0, 3, +1 }, BlockType::TREE_LEAVES);
    struct_tree.push_block({ +1, 3,  0 }, BlockType::TREE_LEAVES);
    struct_tree.push_block({ -1, 3,  0 }, BlockType::TREE_LEAVES);
    struct_tree.push_block({  0, 4, -1 }, BlockType::TREE_LEAVES);
    struct_tree.push_block({  0, 4, +1 }, BlockType::TREE_LEAVES);
    struct_tree.push_block({ +1, 4,  0 }, BlockType::TREE_LEAVES);
    struct_tree.push_block({ -1, 4,  0 }, BlockType::TREE_LEAVES);
    struct_tree.push_block({  0, 5, 0 }, BlockType::TREE_LEAVES);

    begin_ticket_mutex(mutex_world_get_chunk);
    m_chunk_table.insert(chunk_xz, created);
    end_ticket_mutex(mutex_world_get_chunk);

    int32_t height_map[CHUNK_SIZE_X][CHUNK_SIZE_Z];

    this->gen_chunk_height_map(chunk_xz, height_map);

    for(int32_t x = 0; x < CHUNK_SIZE_X; ++x) {
        for(int32_t z = 0; z < CHUNK_SIZE_Z; ++z) {

            int32_t height = height_map[x][z];

            /* Generate stone shape of the terrain */
            for(int32_t y = 0; y < height; ++y) {
                if(y >= CHUNK_SIZE_Y) {
                    assert(false);
                }
                created->set_block({ x, y, z }, BlockType::STONE);
            }

            if(rand() % 84 == 0 && height > ocean_level + 4) {
                this->insert_world_structure(struct_tree, block_position_from_relative({ x, height_map[x][z] + 1, z }, created->get_coords()));
            }

            int32_t y = height;
            while(y >= 0 && (height - y) < 4) {
                if(y <= (this->ocean_level + 1)) {
                    created->set_block({ x, y, z }, BlockType::SAND);
                } else if(y == height) {
                    created->set_block({ x, y, z }, BlockType::DIRT_WITH_GRASS);

                    // grass
                    if((rand() % 26) == 0) {
                        if((y + 1) < CHUNK_SIZE_Y) {
                            if(created->get_block({ x, y + 1, z })->type == BlockType::AIR) {
                                created->set_block({ x, y + 1, z }, BlockType::GRASS);
                            }
                        }
                    }
                } else {
                    created->set_block({ x, y, z }, BlockType::DIRT);
                }
                y -= 1;
            }

            y = ocean_level;
            while(y >= 0) {
                if(created->get_block({ x, y, z})->type != BlockType::AIR) {
                    break;
                }
                created->set_block({ x, y, z }, BlockType::WATER);
                y--;
            }
        }
    }

#if 1
    this->queue_chunk_vao_load(chunk_xz);

    /* @todo */
    if(this->get_chunk(chunk_xz + vec2i{ 1, 0 }, false)) {
        this->queue_chunk_vao_load(chunk_xz + vec2i{ 1, 0 });
    }
    if(this->get_chunk(chunk_xz + vec2i{ -1, 0 }, false)) {
        this->queue_chunk_vao_load(chunk_xz + vec2i{ -1, 0 });
    }
    if(this->get_chunk(chunk_xz + vec2i{ 0, 1 }, false)) {
        this->queue_chunk_vao_load(chunk_xz + vec2i{ 0, 1 });
    }
    if(this->get_chunk(chunk_xz + vec2i{ 0, -1 }, false)) {
        this->queue_chunk_vao_load(chunk_xz + vec2i{ 0, -1 });
    }
#endif

    return created;
}

void World::insert_world_structure(const WorldStructure &structure, const vec3i &origin) {
    for(uint32_t index = 0; index < structure.block_count; ++index) {

        const WorldPosition block_p = WorldPosition::from_block(origin + structure.blocks[index].rel_p);

        Chunk *chunk = this->get_chunk(block_p.chunk, true);
        ASSERT(chunk);
        chunk->set_block(block_p.block_rel, structure.blocks[index].type);

        this->queue_chunk_vao_load(chunk->get_coords());
    }
}

static inline bool is_chunk_in_range(vec2i chunk_xz, vec2i origin, int32_t radius) {
    bool result = chunk_xz.x >= origin.x - radius
        && chunk_xz.x <= origin.x + radius
        && chunk_xz.y >= origin.y - radius
        && chunk_xz.y <= origin.y + radius;
    return result;
}

void World::update_loaded_chunks(float delta_time) {
    const vec2i player_chunk = m_owner->get_player().get_position_chunk();

    std::vector<vec2i> chunks_to_unload;

    ChunkHashTable::Iterator chunk_iter; 
    while(m_chunk_table.iterate_all(chunk_iter)) {
        Chunk *chunk = chunk_iter.value;
        ASSERT(chunk);

        bool in_range = is_chunk_in_range(chunk_iter.key, player_chunk, get_load_radius());
        if(!chunk->m_should_unload && !in_range) {
            chunk->m_should_unload = true;
            chunk->m_unload_timer = 0.0f;
        } else if(chunk->m_should_unload && in_range) {
            chunk->m_should_unload = false;
        }

        if(chunk->m_should_unload) {
            chunk->m_unload_timer += delta_time;
            if(chunk->m_unload_timer >= 5.0f || !is_chunk_in_range(chunk_iter.key, player_chunk, UNLOAD_RADIUS_MAX)) {
                chunks_to_unload.push_back(chunk->get_coords());
                continue;
            }
        }
    }

    begin_ticket_mutex(mutex_chunk_gen_queue);
    begin_ticket_mutex(mutex_chunk_vao_queue);
    begin_ticket_mutex(mutex_chunk_gpu_queue);
    for(const vec2i &chunk_xz : chunks_to_unload) {
        Chunk *chunk = *m_chunk_table.find(chunk_xz);
        chunk->m_chunk_vao.delete_vao();
        chunk->m_water_vao.delete_vao();
        m_chunk_table.remove(chunk_xz);       
 //       fprintf(stdout, "Removed: %d, %d\n", chunk_xz.x, chunk_xz.y);
    }
    end_ticket_mutex(mutex_chunk_gpu_queue);
    end_ticket_mutex(mutex_chunk_vao_queue);
    end_ticket_mutex(mutex_chunk_gen_queue);
}

vec3 real_position_from_block(const vec3i &block) {
    return {
        (float)block.x,
        (float)block.y,
        (float)block.z,
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

vec2i chunk_position_from_real(const vec3 &real) {
    vec3i block = block_position_from_real(real);
    return chunk_position_from_block(block);
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

// https://www.youtube.com/watch?v=fIu_8b2n8ZM
bool ray_plane_intersection(const vec3 &ray_origin, const vec3 &ray_end, const vec3 &plane_p, const vec3 &plane_normal, float &out_k, vec3 &out_p) {
    const vec3  v = ray_end - ray_origin;
    const vec3  w = plane_p - ray_origin;
    const float k = vec3::dot(w, plane_normal) / vec3::dot(v, plane_normal);
    const vec3  p = ray_origin + v * k;

    if(k < 0.0f || k > 1.0f) {
        return false;
    }

    out_k = k;
    out_p = p;
    return true;
}

// https://www.youtube.com/watch?v=EZXz-uPyCyA
bool ray_triangle_intersection(const vec3 &ray_origin, const vec3 &ray_end, const vec3 &tri_a, const vec3 &tri_b, const vec3 &tri_c, float &out_k, vec3 &out_p) {
    vec3 plane_normal = vec3::cross(tri_b - tri_a, tri_c - tri_a);

    float k;
    vec3  p;
    if(!ray_plane_intersection(ray_origin, ray_end, tri_a, plane_normal, k, p)) {
        return false;
    }

    /* Check a */ {
        const vec3 cb = tri_b - tri_c;
        const vec3 ab = tri_b - tri_a;
        const vec3 ap = p - tri_a;

        vec3  v = ab - cb * (vec3::dot(cb, ab) / vec3::dot(cb, cb));
        float a = 1.0f - vec3::dot(v, ap) / vec3::dot(v, ab);

        if(a < 0.0f || a > 1.0f) {
            return false;
        }
    }

    /* Check b */ {
        const vec3 bc = tri_c - tri_b;
        const vec3 ac = tri_c - tri_a;
        const vec3 bp = p - tri_b;

        vec3  v = bc - ac * (vec3::dot(ac, bc) / vec3::dot(ac, ac));
        float b = 1.0f - vec3::dot(v, bp) / vec3::dot(v, bc);

        if(b < 0.0f || b > 1.0f) {
            return false;
        }
    }

    /* Check c */ {
        const vec3 ca = tri_a - tri_c;
        const vec3 ba = tri_a - tri_b;
        const vec3 cp = p - tri_c;

        vec3  v = ca - ba * (vec3::dot(ba, ca) / vec3::dot(ba, ba));
        float c = 1.0f - vec3::dot(v, cp) / vec3::dot(v, ca);

        if(c < 0.0f || c > 1.0f) {
            return false;
        }
    }

    out_k = k;
    out_p = p;
    return true;
}

struct CheckSide {
    BlockSide side;
    vec3i normal;
    vec3  off_a;
    vec3  off_b;
    vec3  off_c;
    vec3  off_d;
};

constexpr static CheckSide sides[6] = {
    /* Y pos */ {
        .side = BlockSide::Y_POS,
        .normal = { 0, 1, 0 },
        .off_a = vec3{ 0.0f, 1.0f, 0.0f },
        .off_b = vec3{ 1.0f, 1.0f, 0.0f },
        .off_c = vec3{ 1.0f, 1.0f, 1.0f },
        .off_d = vec3{ 0.0f, 1.0f, 1.0f },
    },
    /* Y neg */ {
        .side = BlockSide::Y_NEG,
        .normal = { 0, -1, 0 },
        .off_a = vec3{ 0.0f, 0.0f, 0.0f },
        .off_b = vec3{ 1.0f, 0.0f, 0.0f },
        .off_c = vec3{ 1.0f, 0.0f, 1.0f },
        .off_d = vec3{ 0.0f, 0.0f, 1.0f },
    },
    /* X neg */ {
        .side = BlockSide::X_NEG,
        .normal = { -1, 0, 0 },
        .off_a = vec3{ 0.0f, 0.0f, 1.0f },
        .off_b = vec3{ 0.0f, 0.0f, 0.0f },
        .off_c = vec3{ 0.0f, 1.0f, 0.0f },
        .off_d = vec3{ 0.0f, 1.0f, 1.0f },
    },
    /* X pos */ {
        .side = BlockSide::X_POS,
        .normal = { 1, 0, 0 },
        .off_a = vec3{ 1.0f, 0.0f, 1.0f },
        .off_b = vec3{ 1.0f, 0.0f, 0.0f },
        .off_c = vec3{ 1.0f, 1.0f, 0.0f },
        .off_d = vec3{ 1.0f, 1.0f, 1.0f },
    },
    /* Z neg */ {
        .side = BlockSide::Z_NEG,
        .normal = { 0, 0, -1 },
        .off_a = vec3{ 0.0f, 0.0f, 0.0f },
        .off_b = vec3{ 1.0f, 0.0f, 0.0f },
        .off_c = vec3{ 1.0f, 1.0f, 0.0f },
        .off_d = vec3{ 0.0f, 1.0f, 0.0f },
    },
    /* Z pos */ {
        .side = BlockSide::Z_POS,
        .normal = { 0, 0, 1 },
        .off_a = vec3{ 0.0f, 0.0f, 1.0f },
        .off_b = vec3{ 1.0f, 0.0f, 1.0f },
        .off_c = vec3{ 1.0f, 1.0f, 1.0f },
        .off_d = vec3{ 0.0f, 1.0f, 1.0f },
    },
};

RaycastBlockResult raycast_block(World &world, const vec3 &ray_origin, const vec3 &ray_end) {
    RaycastBlockResult result;
    result.found = false;
    result.distance = 100000.0f; // FLT_MAX

    WorldPosition ray_origin_p = WorldPosition::from_real(ray_origin);
    WorldPosition ray_end_p = WorldPosition::from_real(ray_end);

    // @todo Probably there is better way than checking pairs of triangles per side
    // @todo block.y could be out of bounds @todo

    vec3i min = {
        .x = MIN(ray_origin_p.block.x, ray_end_p.block.x),
        .y = MIN(ray_origin_p.block.y, ray_end_p.block.y),
        .z = MIN(ray_origin_p.block.z, ray_end_p.block.z),
    };

    vec3i max = {
        .x = MAX(ray_origin_p.block.x, ray_end_p.block.x),
        .y = MAX(ray_origin_p.block.y, ray_end_p.block.y),
        .z = MAX(ray_origin_p.block.z, ray_end_p.block.z),
    };

    for(int32_t y = min.y; y <= max.y; ++y) {
        for(int32_t x = min.x; x <= max.x; ++x) {
            for(int32_t z = min.z; z <= max.z; ++z) {
                WorldPosition block_p = WorldPosition::from_block({ x, y, z });

                // Getting chunk for every block... @todo
                Chunk *chunk = world.get_chunk(block_p.chunk);
                if(!chunk) {
                    continue;
                }

                Block *block = chunk->get_block(block_p.block_rel);
                if(!block || !(get_block_flags(block->type) & IS_SOLID)) {
                    continue;
                }

                for(int32_t index = 0; index < ARRAY_COUNT(sides); ++index) {
                    CheckSide side = sides[index];
                    vec3 tri_a = block_p.real + side.off_a;
                    vec3 tri_b = block_p.real + side.off_b;
                    vec3 tri_c = block_p.real + side.off_c;
                    vec3 tri_d = block_p.real + side.off_d;

                    if(vec3::dot(vec3::normalize(ray_end - ray_origin), vec3::make(side.normal)) >= 0.0f) {
                        continue;
                    }

                    float distance;
                    vec3  intersection;
                    if(ray_triangle_intersection(ray_origin, ray_end, tri_a, tri_b, tri_c, distance, intersection) || 
                       ray_triangle_intersection(ray_origin, ray_end, tri_c, tri_d, tri_a, distance, intersection)) {
                        if(distance < result.distance) {
                            result.distance = distance;
                            result.intersection = intersection;
                            result.block_p = block_p;
                            result.normal = side.normal;
                            result.side = side.side;
                            result.found = true;
                        }
                    }
                }
            }
        }
    }

    return result;
}

void WorldStructure::push_block(vec3i rel_p, BlockType type) {
    ASSERT(block_count + 1 < ARRAY_COUNT(WorldStructure::blocks)); // @todo Handle
                                                                
    const uint32_t index = block_count++;
    blocks[index].type  = type;
    blocks[index].rel_p = rel_p;
}
