#include "chunk.h"
#include "world.h"
#include "world_utils.h"
#include "chunk_mesh_gen.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stb_image.h>

static inline uint32_t get_block_array_index(const vec3i &rel) {
    ASSERT(is_inside_chunk(rel) && "Block out of bounds!!!!!");
    return rel.x * (CHUNK_SIZE_Y * CHUNK_SIZE_Z) + rel.y * CHUNK_SIZE_Z + rel.z;
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

        for(uint8_t texture = 0; texture < (uint8_t)BlockTexture::_COUNT; ++texture) {
            if(texture >= texture_array.get_count()) {
                fprintf(stderr, "[Error] Not enough texture array slots to fill block textures.\n");
                break;
            }

            const vec2i texture_tile = get_block_texture_tile((BlockTexture)texture);
            const uint32_t rect_x = texture_tile.x * 16;
            const uint32_t rect_y = texture_tile.y * 16;

            rect = copy_image_memory_rect(pixels, width, height, channels, rect_x, rect_y, 16, 16, rect);
            texture_array.set_pixels(texture, rect, 16, 16, TextureDataFormat::RGBA, TextureDataType::UNSIGNED_BYTE);
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

World *Chunk::get_world(void) {
    ASSERT(m_owner != NULL && "Chunk without owner error!.");
    return m_owner;
}

Chunk::Chunk(class World *world, vec2i chunk_xz) {
    ASSERT(world);
    m_owner = world;
    m_chunk_xz = chunk_xz;
    memset(m_blocks, 0, CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * sizeof(BlockType));

    m_should_unload = false;
    m_unload_timer  = 0.0f;

    m_state = ChunkState::GENERATING;
    m_mesh_state = ChunkMeshState::NOT_LOADED;
}

Chunk::~Chunk(void) {
    this->delete_vaos();
    m_owner = NULL;
}

const VertexArray &Chunk::get_vao(void) {
    return m_chunk_vao;
}

const VertexArray &Chunk::get_water_vao(void) {
    return m_water_vao;
}

void Chunk::delete_vaos(void) {
    m_chunk_vao.delete_vao();
    m_water_vao.delete_vao();
}

void Chunk::set_block(const vec3i &rel, BlockType type) {
    if(is_inside_chunk(rel)) {
        uint32_t index = get_block_array_index(rel);
        m_blocks[index] = type;
    }
}

BlockType Chunk::get_block(const vec3i &rel) {
    BlockType block = BlockType::_INVALID;
    if(is_inside_chunk(rel)) {
        uint32_t index = get_block_array_index(rel);
        block = m_blocks[index];
    }
    return block;
}

BlockType Chunk::get_block_neighbour(const vec3i &rel, BlockSide side, bool check_other_chunk) {
    BlockType block = BlockType::_INVALID;
    vec3i nb_rel = rel + get_block_side_dir(side);
    if(is_inside_chunk(nb_rel)) {
        block = m_blocks[nb_rel.x * (CHUNK_SIZE_Y * CHUNK_SIZE_Z) + nb_rel.y * CHUNK_SIZE_Z + nb_rel.z];
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

BlockType *Chunk::get_block_data(void) {
    BlockType *blocks = static_cast<BlockType *>(m_blocks);
    return blocks;
}

vec2i Chunk::get_chunk_xz(void) {
    return m_chunk_xz;
}

void Chunk::set_wait_for_mesh_reload(void) {
    m_mesh_state = ChunkMeshState::WAITING;
}

void Chunk::set_chunk_blocks(BlockType blocks[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z]) {
   memcpy(m_blocks, blocks, CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * sizeof(BlockType)); 
}

ChunkState Chunk::get_chunk_state(void) {
    return m_state;
}

void Chunk::set_chunk_state(ChunkState state) {
    m_state = state;
}

void Chunk::set_mesh(ChunkMeshGenData *gen_data) {
    BufferLayout layout;
    layout.push_attribute("a_packed", 2, BufferDataType::UINT);

    m_chunk_vao.delete_vao();
    m_chunk_vao.create_vao(layout, ArrayBufferUsage::STATIC);
    m_chunk_vao.set_vbo_data(gen_data->chunk.vertices.data(), gen_data->chunk.vertices.size() * sizeof(ChunkVaoVertexPacked));
    m_chunk_vao.set_ibo_data(gen_data->chunk.indices.data(),  gen_data->chunk.indices.size());
    m_chunk_vao.apply_vao_attributes();

    m_water_vao.delete_vao();
    m_water_vao.create_vao(layout, ArrayBufferUsage::STATIC);
    m_water_vao.set_vbo_data(gen_data->water.vertices.data(), gen_data->water.vertices.size() * sizeof(ChunkVaoVertexPacked));
    m_water_vao.set_ibo_data(gen_data->water.indices.data(),  gen_data->water.indices.size());
    m_water_vao.apply_vao_attributes();
}

ChunkMeshState Chunk::get_mesh_state(void) {
    return m_mesh_state;
}

void Chunk::set_mesh_state(ChunkMeshState state) {
    m_mesh_state = state;
}

void Chunk::set_appear_animation(void) {
    m_appear_do_anim = true;
    m_appear_timer = 0.0f;
}

float Chunk::get_chunk_render_offset_y(void) {
    float offset;
    if(m_appear_do_anim && m_mesh_state == ChunkMeshState::LOADED) {
        offset = -(CHUNK_SIZE_Y * (1.0f - m_appear_timer));
    } else {
        offset = 0.0f;
    }
    return offset;
}
