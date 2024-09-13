#include "chunk.h"

#include <stdlib.h>

Block::Block(void) {
    this->set_type(BlockType::AIR);
}

bool Block::is_of_type(BlockType type) {
    return m_type == type;
}

void Block::set_type(BlockType type) {
    m_type = type;
}

Chunk::Chunk(void) {
    for_every_block(x, y, z) {
       Block &block = this->get_block(x, y, z);

       int32_t result = rand() % 4;
       if(result == 2) {
           block.set_type(BlockType::SAND);
       } else {
           block.set_type(BlockType::AIR);
       }
    }
}

Chunk::~Chunk(void) {

}

Block &Chunk::get_block(int32_t rel_x, int32_t rel_y, int32_t rel_z) {
    ASSERT(rel_x >= 0 && rel_x < CHUNK_SIZE_X);
    ASSERT(rel_y >= 0 && rel_y < CHUNK_SIZE_Y);
    ASSERT(rel_z >= 0 && rel_z < CHUNK_SIZE_Z);
    return m_blocks[rel_x][rel_y][rel_z];
}
