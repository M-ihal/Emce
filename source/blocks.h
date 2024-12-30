#pragma once

#include "common.h"
#include "math_types.h"

enum BlockTypeFlags : uint32_t {
    IS_PLACABLE       = 1 << 0,
    IS_SOLID          = 1 << 1,
    IS_NOT_VISIBLE    = 1 << 2,
    IS_NOT_TARGETABLE = 1 << 4,
    HAS_TRANSPARENCY  = 1 << 16,
    
    /* Ignored for CROSS blocks */
    NO_AO      = 1 << 20,
    NO_AO_CAST = 1 << 21,
    CONNECTS_WITH_SAME_TYPE = 1 << 22,
};

enum class BlockSide  : uint8_t { X_NEG, X_POS, Z_NEG, Z_POS, Y_NEG, Y_POS };
enum class BlockShape : uint8_t { NONE, CUBE, CROSS };

/* __VA_ARGS__ specify block textures
 *  for shape_ = NONE -> 0 textures,
 *  for shape_ = CUBE -> 6 textures; in BlockSide order
 *  for shape_ = CROSS -> 1 texture
 * */

#define BLOCK(type_, name_, shape_, flags_, ...) 
#define BLOCKS\
    BLOCK(AIR, "Air",  NONE, IS_NOT_VISIBLE | NO_AO | NO_AO_CAST, { })\
\
    BLOCK(SAND, "Sand", CUBE, IS_SOLID | IS_PLACABLE, { BlockTexture::SAND })\
\
    BLOCK(DIRT, "Dirt", CUBE, IS_SOLID | IS_PLACABLE, { BlockTexture::DIRT })\
\
    BLOCK(DIRT_WITH_GRASS, "Dirt with grass", CUBE, IS_SOLID | IS_PLACABLE,\
            { BlockTexture::DIRT_WITH_GRASS_SIDE, BlockTexture::DIRT_WITH_GRASS_SIDE,\
              BlockTexture::DIRT_WITH_GRASS_SIDE, BlockTexture::DIRT_WITH_GRASS_SIDE,\
              BlockTexture::DIRT, BlockTexture::DIRT_WITH_GRASS_TOP })\
\
    BLOCK(COBBLESTONE, "Cobblestone", CUBE, IS_SOLID | IS_PLACABLE, { BlockTexture::COBBLESTONE })\
\
    BLOCK(STONE, "Stone", CUBE, IS_SOLID | IS_PLACABLE, { BlockTexture::STONE })\
\
    BLOCK(TREE_OAK_LOG, "Oak tree", CUBE, IS_SOLID | IS_PLACABLE,\
            { BlockTexture::TREE_OAK_SIDE, BlockTexture::TREE_OAK_SIDE,\
              BlockTexture::TREE_OAK_SIDE, BlockTexture::TREE_OAK_SIDE,\
              BlockTexture::TREE_OAK_TOP, BlockTexture::TREE_OAK_TOP })\
\
    BLOCK(TREE_OAK_LEAVES, "Oak leaves", CUBE, IS_SOLID | IS_PLACABLE | HAS_TRANSPARENCY, { BlockTexture::TREE_OAK_LEAVES })\
\
    BLOCK(TREE_BIRCH_LOG, "Birch tree", CUBE, IS_SOLID | IS_PLACABLE,\
            { BlockTexture::TREE_BIRCH_SIDE, BlockTexture::TREE_BIRCH_SIDE,\
              BlockTexture::TREE_BIRCH_SIDE, BlockTexture::TREE_BIRCH_SIDE,\
              BlockTexture::TREE_BIRCH_TOP, BlockTexture::TREE_BIRCH_TOP })\
\
    BLOCK(TREE_BIRCH_LEAVES, "Birch leaves", CUBE, IS_SOLID | IS_PLACABLE | HAS_TRANSPARENCY, { BlockTexture::TREE_BIRCH_LEAVES })\
\
    BLOCK(GLASS, "Glass", CUBE, IS_SOLID | IS_PLACABLE | HAS_TRANSPARENCY | NO_AO | NO_AO_CAST | CONNECTS_WITH_SAME_TYPE, { BlockTexture::GLASS })\
\
    BLOCK(GRASS, "Grass", CROSS, IS_PLACABLE | HAS_TRANSPARENCY, { BlockTexture::GRASS })\
\
    BLOCK(WATER, "Water", CUBE, IS_PLACABLE | HAS_TRANSPARENCY | NO_AO_CAST, { BlockTexture::WATER })\
\
    BLOCK(CACTUS, "Cactus", CUBE, IS_SOLID | IS_PLACABLE,\
            { BlockTexture::CACTUS_SIDE, BlockTexture::CACTUS_SIDE,\
              BlockTexture::CACTUS_SIDE, BlockTexture::CACTUS_SIDE,\
              BlockTexture::CACTUS_TOP, BlockTexture::CACTUS_TOP })\
\
    BLOCK(DANDELION, "Dandelion", CROSS, IS_PLACABLE | HAS_TRANSPARENCY, { BlockTexture::DANDELION })\
\
    BLOCK(ROSE, "Rose", CROSS, IS_PLACABLE | HAS_TRANSPARENCY, { BlockTexture::ROSE })\
\
    BLOCK(DEADBUSH, "Deadbush", CROSS, IS_PLACABLE | HAS_TRANSPARENCY, { BlockTexture::DEADBUSH })
#undef BLOCK

#define BLOCK(type_, ...) type_,
enum class BlockType : uint8_t { BLOCKS  _COUNT, _INVALID };
static_assert(sizeof(BlockType) == 1);
#undef BLOCK

constexpr inline bool is_block_type_valid(BlockType type) {
    bool is_valid = (uint8_t)type < (uint8_t)BlockType::_COUNT;
    return is_valid;
}

#define BLOCK(type_, name_, shape_, ...) BlockShape::shape_,
inline constexpr BlockShape block_type_shape[(uint8_t)BlockType::_COUNT] = { BLOCKS };
inline constexpr BlockShape get_block_type_shape(BlockType type) { ASSERT(is_block_type_valid(type)); return block_type_shape[(uint8_t)type]; }
#undef BLOCK

#define BLOCK(type_, name_, shape_, flags_, ...) flags_,
inline constexpr uint32_t block_type_flags[(uint8_t)BlockType::_COUNT] = { BLOCKS };
inline constexpr uint32_t get_block_type_flags(BlockType type) { ASSERT(is_block_type_valid(type)); return block_type_flags[(uint8_t)type]; }
#undef BLOCK

#define BLOCK(type_, name_, ...) name_,
inline const char *block_type_string[(uint8_t)BlockType::_COUNT] = { BLOCKS };
inline const char *get_block_type_string(BlockType type) { ASSERT(is_block_type_valid(type)); return block_type_string[(uint8_t)type]; }
#undef BLOCK

/* Define block textures */

#define DEFINE_TEXTURE(tex_, tile_x_, tile_y_)
#define DEFINE_TEXTURES\
    DEFINE_TEXTURE(SAND, 0, 0)\
    DEFINE_TEXTURE(DIRT, 1, 0)\
    DEFINE_TEXTURE(DIRT_WITH_GRASS_SIDE, 1, 1)\
    DEFINE_TEXTURE(DIRT_WITH_GRASS_TOP, 1, 2)\
    DEFINE_TEXTURE(COBBLESTONE, 2, 0)\
    DEFINE_TEXTURE(STONE, 2, 1)\
    DEFINE_TEXTURE(TREE_OAK_SIDE, 7, 1)\
    DEFINE_TEXTURE(TREE_OAK_TOP, 7, 0)\
    DEFINE_TEXTURE(TREE_OAK_LEAVES, 7, 2)\
    DEFINE_TEXTURE(TREE_BIRCH_SIDE, 8, 1)\
    DEFINE_TEXTURE(TREE_BIRCH_TOP, 8, 0)\
    DEFINE_TEXTURE(TREE_BIRCH_LEAVES, 8, 2)\
    DEFINE_TEXTURE(GLASS, 0, 1)\
    DEFINE_TEXTURE(GRASS, 1, 3)\
    DEFINE_TEXTURE(WATER, 31, 3)\
    DEFINE_TEXTURE(CACTUS_SIDE, 3, 0)\
    DEFINE_TEXTURE(CACTUS_TOP, 3, 1)\
    DEFINE_TEXTURE(DANDELION, 18, 1)\
    DEFINE_TEXTURE(ROSE, 18, 2)\
    DEFINE_TEXTURE(DEADBUSH, 18, 0)
#undef DEFINE_TEXTURE

#define DEFINE_TEXTURE(tex_, ...) tex_,
enum class BlockTexture : uint8_t { DEFINE_TEXTURES  _COUNT, _INVALID };
#undef DEFINE_TEXTURE

constexpr inline bool is_block_texture_valid(BlockTexture tex) {
    bool is_valid = (uint8_t)tex < (uint8_t)BlockTexture::_COUNT;
    return is_valid;
}

#define DEFINE_TEXTURE(tex_, tile_x_, tile_y_) { tile_x_, tile_y_ },
inline constexpr vec2i block_texture_tile[(uint8_t)BlockTexture::_COUNT] = { DEFINE_TEXTURES };
inline constexpr vec2i get_block_texture_tile(BlockTexture tex) { ASSERT(is_block_texture_valid(tex)); return block_texture_tile[(uint8_t)tex]; }
#undef DEFINE_TEXTURE

#define BLOCK(type_, name_, shape_, flags_, ...) case BlockType::type_: { if(BlockShape::shape_ != BlockShape::CUBE) { ASSERT(!"Invalid shape"); return BlockTexture::_INVALID; } BlockTexture textures[] = __VA_ARGS__; ASSERT(ARRAY_COUNT(textures) == 6 || ARRAY_COUNT(textures) == 1); return ARRAY_COUNT(textures) == 1 ? textures[0] : textures[side_index]; }
inline constexpr BlockTexture get_block_cube_texture(BlockType type, BlockSide side) { 
    uint8_t side_index = (uint8_t)side;
    ASSERT(side_index < 6); 

    switch(type) {
        default: ASSERT(!"Invalid block type"); return BlockTexture::_INVALID;
        BLOCKS
    }
}
#undef BLOCK

#define BLOCK(type_, name_, shape_, flags_, ...) case BlockType::type_: { if(BlockShape::shape_ != BlockShape::CROSS) { ASSERT(!"Invalid shape"); return BlockTexture::_INVALID; } BlockTexture textures[] = __VA_ARGS__; ASSERT(ARRAY_COUNT(textures) == 1); return textures[0]; }
inline constexpr BlockTexture get_block_cross_texture(BlockType type) { 
    switch(type) {
        default: ASSERT(!"Invalid block type"); return BlockTexture::_INVALID;
        BLOCKS
    }
}
#undef BLOCK

inline constexpr vec2i get_block_cube_texture_tile(BlockType type, BlockSide side) {
    BlockTexture texture = get_block_cube_texture(type, side);
    return get_block_texture_tile(texture); 
}
