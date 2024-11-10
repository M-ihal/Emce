#include "chunk.h"
#include "world.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glew.h>

#include <stb_image.h>

void init_chunk_vao_gen_data(ChunkVaoGenData &gen_data, uint32_t chunk_reserve, uint32_t water_reserve) {
 //   gen_data.chunk_vertices.initialize(chunk_reserve);
 //   gen_data.chunk_indices .initialize(chunk_reserve);
 //   gen_data.water_vertices.initialize(water_reserve);
 //   gen_data.water_indices .initialize(water_reserve);
}

void free_chunk_vao_gen_data(ChunkVaoGenData &gen_data) {
 //   gen_data.chunk_vertices.destroy();
 //   gen_data.chunk_indices .destroy();
 //   gen_data.water_vertices.destroy();
 //   gen_data.water_indices .destroy();
}

void init_block_texture_array(TextureArray &texture_array, const char *atlas_filepath) {
    stbi_set_flip_vertically_on_load(true);

    int32_t width;
    int32_t height;
    int32_t channels;
    uint8_t *pixels = (uint8_t *)stbi_load(atlas_filepath, &width, &height, &channels, 4);

    if(pixels) {
        // @todo
        ASSERT(channels == 4); 

        uint8_t *rect = (uint8_t *)malloc(16 * 16 * 4);
        ASSERT(rect);

        for(int32_t texture_id = 0; texture_id < BlockTextureID::_TEX_COUNT; ++texture_id) {
            if(texture_id >= texture_array.get_count()) {
                fprintf(stderr, "[Error] Not enough texture array slots to fill block textures.\n");
                break;
            }

            const uint32_t rect_x = block_texture_atlas_positions[texture_id].x * 16;
            const uint32_t rect_y = block_texture_atlas_positions[texture_id].y * 16;

            rect = copy_image_memory_rect(pixels, width, height, channels, rect_x, rect_y, 16, 16, rect);
            texture_array.set_pixels(texture_id, rect, 16, 16, TextureDataFormat::RGBA, TextureDataType::UNSIGNED_BYTE);
        }

        free(rect);
        free(pixels);
    }
}

void init_water_texture_array(TextureArray &texture_array, const char *water_filepath) {
    stbi_set_flip_vertically_on_load(true);

    int32_t width;
    int32_t height;
    int32_t channels;
    uint8_t *pixels = (uint8_t *)stbi_load(water_filepath, &width, &height, &channels, 4);

    if(pixels) {
        /* Assert that there are 8 water frames in the image */
        uint32_t water_frames = 32;
        ASSERT(water_frames == (height / 16));

        // @todo
        ASSERT(channels == 4); 

        uint8_t *rect = (uint8_t *)malloc(16 * 16 * 4);
        ASSERT(rect);

        for(int32_t water_frame = 0; water_frame < water_frames; ++water_frame) {
            const uint32_t rect_x = 0;
            const uint32_t rect_y = water_frame * 16;

            rect = copy_image_memory_rect(pixels, width, height, channels, rect_x, rect_y, 16, 16, rect);
            texture_array.set_pixels(water_frame, rect, 16, 16, TextureDataFormat::RGBA, TextureDataType::UNSIGNED_BYTE);
        }

        free(rect);
        free(pixels);
    }
}

Chunk::Chunk(class World *world, vec2i chunk_xz) {
    ASSERT(world);
    m_owner = world;
    m_chunk_xz = chunk_xz;
    memset(m_blocks, 0, CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * sizeof(Block));

    m_should_unload = false;
    m_unload_timer  = 0.0f;
}

Chunk::~Chunk(void) {
    m_chunk_vao.delete_vao();
    m_owner = NULL;
}

const VertexArray &Chunk::get_vao(void) {
    return m_chunk_vao;
}

const VertexArray &Chunk::get_water_vao(void) {
    return m_water_vao;
}

void Chunk::set_block(const vec3i &rel, BlockType type) {
    if(is_inside_chunk(rel)) {
        m_blocks[rel.x][rel.y][rel.z].type = type;
    }
}

Block *Chunk::get_block(const vec3i &rel) {
    Block *block = NULL;
    if(is_inside_chunk(rel)) {
        block = &m_blocks[rel.x][rel.y][rel.z];
    }
    return block;
}

Block *Chunk::get_block_neighbour(const vec3i &rel, BlockSide side, bool check_other_chunk) {
    Block *block = NULL;
    vec3i nb_rel = rel + get_block_side_dir(side);
    if(is_inside_chunk(nb_rel)) {
        block = &m_blocks[nb_rel.x][nb_rel.y][nb_rel.z];
    } else if(check_other_chunk && nb_rel.y >= 0 && nb_rel.y < CHUNK_SIZE_Y) {
        const vec3i nb_abs = block_position_from_relative(nb_rel, m_chunk_xz);
        WorldPosition nb_p = WorldPosition::from_block(nb_abs);
        Chunk *nb_chunk = this->m_owner->get_chunk(nb_p.chunk);
        if(nb_chunk != NULL) {
            block = nb_chunk->get_block(nb_p.block_rel);
        }
    }
    return block;
}

vec2i Chunk::get_coords(void) {
    return m_chunk_xz;
}

void Chunk::gen_vao(ChunkVaoGenData &gen_data) {
    BufferLayout layout;
    layout.push_attribute("a_position",  3, BufferDataType::FLOAT);
    layout.push_attribute("a_normal",    3, BufferDataType::FLOAT);
    layout.push_attribute("a_tex_coord", 2, BufferDataType::FLOAT);
    layout.push_attribute("a_tex_slot", 1, BufferDataType::FLOAT);

    m_chunk_vao.delete_vao();
    m_chunk_vao.create_vao(layout, ArrayBufferUsage::STATIC);
    m_chunk_vao.set_vbo_data(gen_data.chunk_vertices.data(), gen_data.chunk_vertices.size() * sizeof(ChunkVaoVertex));
    m_chunk_vao.set_ibo_data(gen_data.chunk_indices.data(),  gen_data.chunk_indices.size());
    m_chunk_vao.apply_vao_attributes();

    m_water_vao.delete_vao();
    m_water_vao.create_vao(layout, ArrayBufferUsage::STATIC);
    m_water_vao.set_vbo_data(gen_data.water_vertices.data(), gen_data.water_vertices.size() * sizeof(ChunkVaoVertex));
    m_water_vao.set_ibo_data(gen_data.water_indices.data(),  gen_data.water_indices.size());
    m_water_vao.apply_vao_attributes();
}

static constexpr void calc_atlas_tex_coords(int32_t x, int32_t y, ChunkVaoVertex v[4]) {
#if 0
    v[0].tex_coord = vec2{ x * 16.0f / 512.0f, y * 16.0f / 512.0f };
    v[1].tex_coord = vec2{ (x + 1) * 16.0f / 512.0f, y * 16.0f / 512.0f };
    v[2].tex_coord = vec2{ (x + 1) * 16.0f / 512.0f, (y + 1) * 16.0f / 512.0f };
    v[3].tex_coord = vec2{ x * 16.0f / 512.0f, (y + 1) * 16.0f / 512.0f };
#else

#endif
}

static void push_block_vao_quad(ChunkVaoVertex quad_vertices[4], std::vector<ChunkVaoVertex> &vertices, std::vector<uint32_t> &indices) {
    const int32_t start = vertices.size();

    vertices.push_back(quad_vertices[0]);
    vertices.push_back(quad_vertices[1]);
    vertices.push_back(quad_vertices[2]);
    vertices.push_back(quad_vertices[3]);

    indices.push_back(start + 0);
    indices.push_back(start + 1);
    indices.push_back(start + 2);
    indices.push_back(start + 2);
    indices.push_back(start + 3);
    indices.push_back(start + 0);
}

static void fill_block_cube_vao_vertex(ChunkVaoVertex v[4], BlockSide side, const vec3i &offset) {
    switch(side) {
        default: INVALID_CODE_PATH; break;

        case BlockSide::Z_POS: {
            v[0].position = vec3{ 0.0f, 0.0f, 1.0f };
            v[1].position = vec3{ 1.0f, 0.0f, 1.0f };
            v[2].position = vec3{ 1.0f, 1.0f, 1.0f };
            v[3].position = vec3{ 0.0f, 1.0f, 1.0f };
            v[0].normal = vec3{ 0.0f, 0.0f, 1.0f };
            v[1].normal = vec3{ 0.0f, 0.0f, 1.0f };
            v[2].normal = vec3{ 0.0f, 0.0f, 1.0f };
            v[3].normal = vec3{ 0.0f, 0.0f, 1.0f };
        } break;

        case BlockSide::Z_NEG: {
            v[0].position = vec3{ 1.0f, 0.0f, 0.0f };
            v[1].position = vec3{ 0.0f, 0.0f, 0.0f };
            v[2].position = vec3{ 0.0f, 1.0f, 0.0f };
            v[3].position = vec3{ 1.0f, 1.0f, 0.0f };
            v[0].normal = vec3{ 0.0f, 0.0f, -1.0f };
            v[1].normal = vec3{ 0.0f, 0.0f, -1.0f };
            v[2].normal = vec3{ 0.0f, 0.0f, -1.0f };
            v[3].normal = vec3{ 0.0f, 0.0f, -1.0f };
        } break;

        case BlockSide::X_POS: {
            v[0].position = vec3{ 1.0f, 0.0f, 1.0f };
            v[1].position = vec3{ 1.0f, 0.0f, 0.0f };
            v[2].position = vec3{ 1.0f, 1.0f, 0.0f };
            v[3].position = vec3{ 1.0f, 1.0f, 1.0f };
            v[0].normal = vec3{ 1.0f, 0.0f, 0.0f };
            v[1].normal = vec3{ 1.0f, 0.0f, 0.0f };
            v[2].normal = vec3{ 1.0f, 0.0f, 0.0f };
            v[3].normal = vec3{ 1.0f, 0.0f, 0.0f };
        } break;

        case BlockSide::X_NEG: {
            v[0].position = vec3{ 0.0f, 0.0f, 0.0f };
            v[1].position = vec3{ 0.0f, 0.0f, 1.0f };
            v[2].position = vec3{ 0.0f, 1.0f, 1.0f };
            v[3].position = vec3{ 0.0f, 1.0f, 0.0f };
            v[0].normal = vec3{ -1.0f, 0.0f, 0.0f };
            v[1].normal = vec3{ -1.0f, 0.0f, 0.0f };
            v[2].normal = vec3{ -1.0f, 0.0f, 0.0f };
            v[3].normal = vec3{ -1.0f, 0.0f, 0.0f };
        } break;

        case BlockSide::Y_POS: {
            v[0].position = vec3{ 0.0f, 1.0f, 1.0f };
            v[1].position = vec3{ 1.0f, 1.0f, 1.0f };
            v[2].position = vec3{ 1.0f, 1.0f, 0.0f };
            v[3].position = vec3{ 0.0f, 1.0f, 0.0f };
            v[0].normal = vec3{ 0.0f, 1.0f, 0.0f };
            v[1].normal = vec3{ 0.0f, 1.0f, 0.0f };
            v[2].normal = vec3{ 0.0f, 1.0f, 0.0f };
            v[3].normal = vec3{ 0.0f, 1.0f, 0.0f };
        } break;

        case BlockSide::Y_NEG: {
            v[0].position = vec3{ 0.0f, 0.0f, 0.0f };
            v[1].position = vec3{ 1.0f, 0.0f, 0.0f };
            v[2].position = vec3{ 1.0f, 0.0f, 1.0f };
            v[3].position = vec3{ 0.0f, 0.0f, 1.0f };
            v[0].normal = vec3{ 0.0f, -1.0f, 0.0f };
            v[1].normal = vec3{ 0.0f, -1.0f, 0.0f };
            v[2].normal = vec3{ 0.0f, -1.0f, 0.0f };
            v[3].normal = vec3{ 0.0f, -1.0f, 0.0f };
        } break;
    }

    v[0].position += vec3::make(offset);
    v[1].position += vec3::make(offset);
    v[2].position += vec3::make(offset);
    v[3].position += vec3::make(offset);
}

static void fill_block_cube_vao_tex_coords(ChunkVaoVertex v[4], BlockType type, BlockSide side) {
    v[0].tex_coord = { 0.0f, 0.0f };
    v[1].tex_coord = { 1.0f, 0.0f };
    v[2].tex_coord = { 1.0f, 1.0f };
    v[3].tex_coord = { 0.0f, 1.0f };

    BlockTextureID texture_id = TEX_SAND;

    switch(type) {
        default: INVALID_CODE_PATH; break;

        case BlockType::SAND: {
            texture_id = TEX_SAND;
            calc_atlas_tex_coords(0, 0, v);
        } break;

        case BlockType::DIRT: {
            texture_id = TEX_DIRT;
            calc_atlas_tex_coords(1, 0, v);
        } break;

        case BlockType::DIRT_WITH_GRASS: {
            switch(side) {
                case BlockSide::Y_NEG: {
                    texture_id = TEX_DIRT;
                    calc_atlas_tex_coords(1, 0, v);
                } break;
                case BlockSide::Y_POS: {
                    calc_atlas_tex_coords(1, 2, v);
                    texture_id = TEX_DIRT_WITH_GRASS_TOP;
                } break;
                case BlockSide::X_POS:
                case BlockSide::X_NEG:
                case BlockSide::Z_POS:
                case BlockSide::Z_NEG: {
                    texture_id = TEX_DIRT_WITH_GRASS_SIDE;
                    calc_atlas_tex_coords(1, 1, v);
                } break;
            }
        } break;

        case BlockType::COBBLESTONE: {
            texture_id = TEX_COBBLESTONE;
            calc_atlas_tex_coords(2, 0, v);
        } break;

        case BlockType::STONE: {
            texture_id = TEX_STONE;
            calc_atlas_tex_coords(2, 1, v);
        } break;

        case BlockType::TREE_LOG: {
            switch(side) {
                case BlockSide::Y_POS:
                case BlockSide::Y_NEG: {
                    texture_id = TEX_TREE_LOG_TOP;
                    calc_atlas_tex_coords(7, 0, v);
                } break;

                case BlockSide::X_POS:
                case BlockSide::X_NEG:
                case BlockSide::Z_POS:
                case BlockSide::Z_NEG: {
                    texture_id = TEX_TREE_LOG_SIDE;
                    calc_atlas_tex_coords(7, 1, v);
                } break;
            };
        } break;

        case BlockType::TREE_LEAVES: {
            texture_id = TEX_TREE_LEAVES;
            calc_atlas_tex_coords(7, 2, v);
        } break;
        
        case BlockType::GLASS: {
            texture_id = TEX_GLASS;
            calc_atlas_tex_coords(0, 1, v);
        } break;

        case BlockType::WATER: {
            texture_id = TEX_WATER;
            calc_atlas_tex_coords(31, 0, v);
        } break;
    }

    v[0].tex_slot = (float)texture_id;
    v[1].tex_slot = (float)texture_id;
    v[2].tex_slot = (float)texture_id;
    v[3].tex_slot = (float)texture_id;
}

static void fill_block_cross_vao_vertex(ChunkVaoVertex v[2][4], const vec3i &offset) {
    v[0][0].position = vec3{ 0.0f, 0.0f, 0.0f };
    v[0][1].position = vec3{ 1.0f, 0.0f, 1.0f };
    v[0][2].position = vec3{ 1.0f, 1.0f, 1.0f };
    v[0][3].position = vec3{ 0.0f, 1.0f, 0.0f };

    v[1][0].position = vec3{ 0.0f, 0.0f, 1.0f };
    v[1][1].position = vec3{ 1.0f, 0.0f, 0.0f };
    v[1][2].position = vec3{ 1.0f, 1.0f, 0.0f };
    v[1][3].position = vec3{ 0.0f, 1.0f, 1.0f };

    v[0][0].position += vec3::make(offset);
    v[0][1].position += vec3::make(offset);
    v[0][2].position += vec3::make(offset);
    v[0][3].position += vec3::make(offset);

    v[1][0].position += vec3::make(offset);
    v[1][1].position += vec3::make(offset);
    v[1][2].position += vec3::make(offset);
    v[1][3].position += vec3::make(offset);

    v[0][0].normal = vec3::normalize({ 1.0f, 0.0f, 1.0f });
    v[0][1].normal = vec3::normalize({ 1.0f, 0.0f, 1.0f });
    v[0][2].normal = vec3::normalize({ 1.0f, 0.0f, 1.0f });
    v[0][3].normal = vec3::normalize({ 1.0f, 0.0f, 1.0f });

    v[1][0].normal = vec3::normalize({ 1.0f, 0.0f, -1.0f });
    v[1][1].normal = vec3::normalize({ 1.0f, 0.0f, -1.0f });
    v[1][2].normal = vec3::normalize({ 1.0f, 0.0f, -1.0f });
    v[1][3].normal = vec3::normalize({ 1.0f, 0.0f, -1.0f });
}

static void fill_block_cross_vao_tex_coords(ChunkVaoVertex v[2][4], BlockType type) {
    // @todo BlockTextureID texture_id = BlockTextureID::TEX_MISSING;
    
    v[0][0].tex_coord = { 0.0f, 0.0f };
    v[0][1].tex_coord = { 1.0f, 0.0f };
    v[0][2].tex_coord = { 1.0f, 1.0f };
    v[0][3].tex_coord = { 0.0f, 1.0f };

    v[1][0].tex_coord = { 0.0f, 0.0f };
    v[1][1].tex_coord = { 1.0f, 0.0f };
    v[1][2].tex_coord = { 1.0f, 1.0f };
    v[1][3].tex_coord = { 0.0f, 1.0f };

    BlockTextureID texture_id = TEX_SAND;

    switch(type) {
        default: break;

        case BlockType::GRASS: {
            texture_id = BlockTextureID::TEX_GRASS;
        } break;
    }

    v[0][0].tex_slot = (float)texture_id;
    v[0][1].tex_slot = (float)texture_id;
    v[0][2].tex_slot = (float)texture_id;
    v[0][3].tex_slot = (float)texture_id;

    v[1][0].tex_slot = (float)texture_id;
    v[1][1].tex_slot = (float)texture_id;
    v[1][2].tex_slot = (float)texture_id;
    v[1][3].tex_slot = (float)texture_id;
}

static void push_block_vao_data(BlockType type, std::vector<ChunkVaoVertex> &vertices, std::vector<uint32_t> &indices, const vec3i &block_rel, bool opaque_neighbours[BLOCK_SIDE_COUNT]) {
    enum : int32_t { CUBE, CROSS, NOT_SET } block_vis;

    switch(type) {
        default: {
            INVALID_CODE_PATH;
        } break;

        case BlockType::SAND:
        case BlockType::DIRT:
        case BlockType::DIRT_WITH_GRASS:
        case BlockType::COBBLESTONE:
        case BlockType::STONE:
        case BlockType::TREE_LOG:
        case BlockType::TREE_LEAVES:
        case BlockType::GLASS:
        case BlockType::WATER: {
            block_vis = CUBE;
        } break;

        case BlockType::GRASS: {
            block_vis = CROSS;
        } break;
    }

    switch(block_vis) {
        default: {
            INVALID_CODE_PATH;
        } break;

        case CUBE: {
            for(int32_t side = 0; side < BLOCK_SIDE_COUNT; ++side) {
                ChunkVaoVertex v[4] = { };
                if(!opaque_neighbours[side]) {
                    fill_block_cube_vao_vertex(v, (BlockSide)side, block_rel);
                    fill_block_cube_vao_tex_coords(v, type, (BlockSide)side);
                    push_block_vao_quad(v, vertices, indices);
                }
            }
        } break;

        case CROSS: {
            ChunkVaoVertex v[2][4] = { };
            fill_block_cross_vao_vertex(v, block_rel);
            fill_block_cross_vao_tex_coords(v, type);
            push_block_vao_quad(v[0], vertices, indices);
            push_block_vao_quad(v[1], vertices, indices);
        } break;
    }
}

static void push_water_vao_data(std::vector<ChunkVaoVertex> &vertices, std::vector<uint32_t> &indices, const vec3i &block_rel, bool opaque_neighbours[BLOCK_SIDE_COUNT]) {
    for(int32_t side = 0; side < BLOCK_SIDE_COUNT; ++side) {
        ChunkVaoVertex v[4] = { };

        if(!opaque_neighbours[side]) {

            fill_block_cube_vao_vertex(v, (BlockSide)side, block_rel);

            v[0].tex_coord = { 0.0f, 0.0f };
            v[1].tex_coord = { 1.0f, 0.0f };
            v[2].tex_coord = { 1.0f, 1.0f };
            v[3].tex_coord = { 0.0f, 1.0f };

            /* Tex slot in water means water frame */
            v[0].tex_slot = 0.0f;
            v[1].tex_slot = 0.0f;
            v[2].tex_slot = 0.0f;
            v[3].tex_slot = 0.0f;

            const int32_t start = vertices.size();

            vertices.push_back(v[0]);
            vertices.push_back(v[1]);
            vertices.push_back(v[2]);
            vertices.push_back(v[3]);

            indices.push_back(start + 0);
            indices.push_back(start + 1);
            indices.push_back(start + 2);
            indices.push_back(start + 2);
            indices.push_back(start + 3);
            indices.push_back(start + 0);
        }
    }
}

void gen_single_block_vao_data(BlockType type, std::vector<ChunkVaoVertex> &vertices, std::vector<uint32_t> &indices) {
    bool opaques[BLOCK_SIDE_COUNT] = {
        false, false, false, 
        false, false, false
    };

    push_block_vao_data(type, vertices, indices, { 0, 0, 0 }, opaques);
    for(uint32_t index = 0; index < vertices.size(); ++index) {
        ChunkVaoVertex &v = vertices[index];
        v.position -= vec3::make(0.5f);
    }
}

ChunkVaoGenData Chunk::gen_vao_data(void) {
    ChunkVaoGenData gen_data = { .chunk = this->m_chunk_xz };
    init_chunk_vao_gen_data(gen_data, 186000, 25000);

    // fprintf(stdout, "Allocating!\n");

    /* Generate general vao data */
    for_every_block(x, y, z) {
        const vec3i block_rel = { x, y, z };

        Block *block = this->get_block(block_rel);
        ASSERT(block);

        /* Water goes to different vao */
        if(block->type == BlockType::WATER) {
            continue;
        }

        const uint32_t flags = get_block_flags(block->type);
        if(flags & IS_NOT_VISIBLE) {
            continue;
        }

        // @TODO Change the name of *opaque_neighbours* to something like _GENERATE_SIDE)

        /* Get array of opaque neighbouring blocks */
        bool opaque_neighbours[BLOCK_SIDE_COUNT] = { };
        for(int32_t side = 0; side < BLOCK_SIDE_COUNT; ++side) {
            Block *nb = this->get_block_neighbour(block_rel, (BlockSide)side, true);
            if(nb != NULL) {
                uint32_t nb_flags = get_block_flags(nb->type);
                opaque_neighbours[side] = !(nb_flags & IS_NOT_VISIBLE) && !(nb_flags & HAS_TRANSPARENCY);

                if(nb->type == BlockType::WATER) {
                    opaque_neighbours[side] = false;
                }

                if(flags & HAS_TRANSPARENCY && nb_flags & HAS_TRANSPARENCY && !(nb_flags & IS_NOT_VISIBLE)) {
                    opaque_neighbours[side] = true;
                }
            }       
        }

        push_block_vao_data(block->type, gen_data.chunk_vertices, gen_data.chunk_indices, block_rel, opaque_neighbours);
    }

    /* Generate water vao data */
    for_every_block(x, y, z) {
        const vec3i block_rel = { x, y, z };

        Block *block = this->get_block(block_rel);
        ASSERT(block);

        if(block->type != BlockType::WATER) {
            continue;
        }
        
        bool neighbours[8] = { };
        for(int32_t side = 0; side < BLOCK_SIDE_COUNT; ++side) {
            Block *nb = this->get_block_neighbour(block_rel, (BlockSide)side, true);

            if((side == (int)BlockSide::Y_POS || side == (int)BlockSide::Y_NEG) && nb->type == BlockType::WATER) {
                int x = 21313;
            }

            if(nb != NULL) {
                uint32_t nb_flags = get_block_flags(nb->type);
                neighbours[side] = !(nb_flags & IS_NOT_VISIBLE) && !(nb_flags & HAS_TRANSPARENCY);


                if(nb->type == BlockType::WATER) {
                    neighbours[side] = true;
                }
            }
        }

        push_water_vao_data(gen_data.water_vertices, gen_data.water_indices, block_rel, neighbours);
    }

 //   fprintf(stdout, "COUNT C: %u, COUNT W: %u\n", gen_data.chunk_vertices.get_count(), gen_data.water_vertices.get_count());

    return std::move(gen_data);
}

