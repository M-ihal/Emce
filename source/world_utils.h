#pragma once

#include "math_types.h"
#include "chunk.h"

bool is_chunk_in_range(vec2i chunk_xz, vec2i origin, int32_t radius);

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

