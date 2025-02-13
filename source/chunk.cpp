#include "chunk.h"
#include "world.h"

World *Chunk::get_world(void) {
    ASSERT(m_owner != NULL && "Chunk without owner error!.");
    return m_owner;
}

Chunk::Chunk(class World *world, vec2i chunk_xz) {
    ASSERT(world);
    m_owner = world;
    m_chunk_coords = chunk_xz;
    memset(m_blocks, 0, CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * sizeof(BlockType));

    m_state = ChunkState::GENERATING;
    m_mesh_state = ChunkMeshState::WAIT_FOR_MESHING;
}

Chunk::~Chunk(void) {
    m_chunk_vao.delete_vao();
    m_water_vao.delete_vao();
    m_owner = NULL;
}

const VertexArray &Chunk::get_vao(void) {
    return m_chunk_vao;
}

const VertexArray &Chunk::get_water_vao(void) {
    return m_water_vao;
}

vec2i Chunk::get_chunk_coords(void) {
    return m_chunk_coords;
}

void Chunk::set_block(const vec3i &rel, BlockType type) {
    ASSERT(is_block_type_valid(type));
    if(is_block_type_valid(type) && is_inside_chunk(rel)) {
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

void Chunk::copy_blocks_into(BlockType blocks[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z]) {
   memcpy(m_blocks, blocks, CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * sizeof(BlockType)); 
}

void Chunk::copy_blocks_from(BlockType blocks[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z]) {
   memcpy(blocks, m_blocks, CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * sizeof(BlockType)); 
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

bool Chunk::should_build_mesh(void) {
    return m_state == ChunkState::GENERATED
        && m_owner->chunk_neighbours_generated(m_chunk_coords)
        && (m_mesh_state == ChunkMeshState::WAIT_FOR_MESHING || m_mesh_state == ChunkMeshState::WAIT_FOR_MESHING_HIGH_PRIORITY);
}

uint32_t Chunk::get_mesh_build_counter(void) {
    return m_mesh_build_counter;
}

uint32_t Chunk::get_mesh_build_counter_next(void) {
    return ++m_mesh_build_counter;
}

