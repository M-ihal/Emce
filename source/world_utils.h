#pragma once

#include "math_types.h"
#include "chunk.h"

class Camera;

bool is_chunk_in_range(vec2i chunk_xz, vec2i origin, int32_t radius);
bool is_chunk_in_frustum(double frustum[6][4], const vec3d &chunk_origin_rel);
bool is_block_on_x_neg_edge(const vec3i &block_rel);
bool is_block_on_x_pos_edge(const vec3i &block_rel);
bool is_block_on_z_neg_edge(const vec3i &block_rel);
bool is_block_on_z_pos_edge(const vec3i &block_rel);

struct WorldPosition {
    static WorldPosition from_real(const vec3d &real);
    static WorldPosition from_block(const vec3i &block);
    vec3d real;
    vec3i block;
    vec3i block_rel;
    vec2i chunk;
};

/* block's real position is it's origin (0,0,0 corner) */
vec3d real_position_from_block(const vec3i &block);
vec3d real_position_from_chunk(const vec2i &chunk);
vec3i block_position_from_real(const vec3d &real);
vec2i chunk_position_from_block(const vec3i &block);
vec2i chunk_position_from_real(const vec3d &real);
vec3i block_relative_from_block(const vec3i &block);
vec3i block_position_from_relative(const vec3i &block_rel, const vec2i &chunk);
void  calc_overlapping_blocks(vec3d pos, vec3d size, WorldPosition &min, WorldPosition &max);

vec3i     get_block_side_normal(BlockSide side);
BlockSide get_block_side_from_normal(const vec3i &normal);

struct RaycastBlockResult {
    bool found;
    vec3i normal;
    vec3d intersection;
    double distance;
    BlockSide side;
    WorldPosition block_p;
};

RaycastBlockResult raycast_block(World &world, const vec3d &ray_origin, const vec3d &ray_end);
bool ray_plane_intersection(const vec3d &ray_origin, const vec3d &ray_end, const vec3d &plane_p, const vec3d &plane_normal, double &out_k, vec3d &out_p);
bool ray_triangle_intersection(const vec3d &ray_origin, const vec3d &ray_end, const vec3d &tri_a, const vec3d &tri_b, const vec3d &tri_c, double &out_k, vec3d &out_p);
