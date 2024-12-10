#include "world_utils.h"
#include "world.h"
#include "camera.h"

bool is_chunk_in_range(vec2i chunk_xz, vec2i origin, int32_t radius) {
    bool result = chunk_xz.x >= origin.x - radius
        && chunk_xz.x <= origin.x + radius
        && chunk_xz.y >= origin.y - radius
        && chunk_xz.y <= origin.y + radius;
    return result;
}

static bool test_plane(double plane[4], const vec3d &min, const vec3d &max) {
    double d = 0.0;
    if(plane[0] > 0.0) { d += max.x * plane[0]; } else { d += min.x * plane[0]; }
    if(plane[1] > 0.0) { d += max.y * plane[1]; } else { d += min.y * plane[1]; }
    if(plane[2] > 0.0) { d += max.z * plane[2]; } else { d += min.z * plane[2]; }
    return (d + plane[3]) >= 0.0;
}

bool is_chunk_in_frustum(double frustum[6][4], const vec3d &chunk_origin_rel) {
    const vec3d chunk_max = chunk_origin_rel + vec3d{ CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z };

    /* Check if chunk is outside of frustum */
    bool in_frustum = true;
    for(int plane_index = 0; plane_index < 5; ++plane_index) {
        if(!test_plane(frustum[plane_index], chunk_origin_rel, chunk_max)) {
            in_frustum = false;
            break;
        }
    }
    return in_frustum;
}

bool is_inside_chunk(const vec3i &block_rel) {
    return block_rel.x >= 0 && block_rel.x < CHUNK_SIZE_X
        && block_rel.y >= 0 && block_rel.y < CHUNK_SIZE_Y
        && block_rel.z >= 0 && block_rel.z < CHUNK_SIZE_Z;
}

bool is_block_on_x_neg_edge(const vec3i &block_rel) {
    return block_rel.x == 0;
}

bool is_block_on_x_pos_edge(const vec3i &block_rel) {
    return block_rel.x == CHUNK_SIZE_X - 1;
}

bool is_block_on_z_neg_edge(const vec3i &block_rel) {
    return block_rel.z == 0;
}

bool is_block_on_z_pos_edge(const vec3i &block_rel) {
    return block_rel.z == CHUNK_SIZE_Z - 1;
}

vec3d real_position_from_block(const vec3i &block) {
    return {
        (double)block.x,
        (double)block.y,
        (double)block.z,
    };
}

vec3d real_position_from_chunk(const vec2i &chunk) {
    return {
        (double)(chunk.x * CHUNK_SIZE_X),
        0.0,
        (double)(chunk.y * CHUNK_SIZE_Z)
    };
}

vec3i block_position_from_real(const vec3d &real) {
    return {
        (int32_t)floor(real.x),
        (int32_t)floor(real.y),
        (int32_t)floor(real.z),
    };
}

vec2i chunk_position_from_block(const vec3i &block) {
    return {
        (int32_t)floor((double)block.x / (double)CHUNK_SIZE_X),
        (int32_t)floor((double)block.z / (double)CHUNK_SIZE_Z)
    };
}

vec2i chunk_position_from_real(const vec3d &real) {
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

vec3i block_position_from_relative(const vec3i &block_rel, const vec2i &chunk) {
    return vec3i{
        .x = chunk.x >= 0 ? chunk.x * CHUNK_SIZE_X + block_rel.x : chunk.x * CHUNK_SIZE_X + block_rel.x,
        .y = block_rel.y,
        .z = chunk.y >= 0 ? chunk.y * CHUNK_SIZE_Z + block_rel.z : chunk.y * CHUNK_SIZE_Z + block_rel.z,
    };
}

void calc_overlapping_blocks(vec3d pos, vec3d size, WorldPosition &min, WorldPosition &max) {
    vec3i block_min_p = block_position_from_real(pos);
    vec3i block_max_p = block_position_from_real(pos + size);
    min = WorldPosition::from_block(block_min_p);
    max = WorldPosition::from_block(block_max_p);
}

vec3i get_block_side_normal(BlockSide side) {
    switch(side) {
        case BlockSide::X_NEG: return vec3i{ -1,  0,  0 };
        case BlockSide::X_POS: return vec3i{ +1,  0,  0 };
        case BlockSide::Z_NEG: return vec3i{  0,  0, -1 };
        case BlockSide::Z_POS: return vec3i{  0,  0,  1 };
        case BlockSide::Y_NEG: return vec3i{  0, -1,  0 };
        case BlockSide::Y_POS: return vec3i{  0, +1,  0 };
    }
    INVALID_CODE_PATH;
    return { };
}

BlockSide get_block_side_from_normal(const vec3i &normal) {
    if(normal == vec3i{ -1, 0, 0 }) {
        return BlockSide::X_NEG;
    } else if(normal == vec3i{ 1, 0, 0 }) {
        return BlockSide::X_POS;
    } if(normal == vec3i{ 0, 0, -1 }) {
        return BlockSide::Z_NEG;
    } else if(normal == vec3i{ 0, 0, 1 }) {
        return BlockSide::Z_POS;
    } else if(normal == vec3i{ 0, -1, 0 }) {
        return BlockSide::Y_NEG;
    } else if(normal == vec3i{ 0, 1, 0 }) {
        return BlockSide::Y_POS;
    }

    INVALID_CODE_PATH;
    return (BlockSide)0;
}

WorldPosition WorldPosition::from_real(const vec3d &real) {
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
bool ray_plane_intersection(const vec3d &ray_origin, const vec3d &ray_end, const vec3d &plane_p, const vec3d &plane_normal, double &out_k, vec3d &out_p) {
    const vec3d v = ray_end - ray_origin;
    const vec3d w = plane_p - ray_origin;
    const double k = vec3d::dot(w, plane_normal) / vec3d::dot(v, plane_normal);
    const vec3d p = ray_origin + v * k;

    if(k < 0.0 || k > 1.0) {
        return false;
    }

    out_k = k;
    out_p = p;
    return true;
}

// https://www.youtube.com/watch?v=EZXz-uPyCyA
bool ray_triangle_intersection(const vec3d &ray_origin, const vec3d &ray_end, const vec3d &tri_a, const vec3d &tri_b, const vec3d &tri_c, double &out_k, vec3d &out_p) {
    vec3d plane_normal = vec3d::cross(tri_b - tri_a, tri_c - tri_a);

    double k;
    vec3d  p;
    if(!ray_plane_intersection(ray_origin, ray_end, tri_a, plane_normal, k, p)) {
        return false;
    }

    /* Check a */ {
        const vec3d cb = tri_b - tri_c;
        const vec3d ab = tri_b - tri_a;
        const vec3d ap = p - tri_a;

        vec3d  v = ab - cb * (vec3d::dot(cb, ab) / vec3d::dot(cb, cb));
        double a = 1.0 - vec3d::dot(v, ap) / vec3d::dot(v, ab);

        if(a < 0.0 || a > 1.0) {
            return false;
        }
    }

    /* Check b */ {
        const vec3d bc = tri_c - tri_b;
        const vec3d ac = tri_c - tri_a;
        const vec3d bp = p - tri_b;

        vec3d  v = bc - ac * (vec3d::dot(ac, bc) / vec3d::dot(ac, ac));
        double b = 1.0 - vec3d::dot(v, bp) / vec3d::dot(v, bc);

        if(b < 0.0 || b > 1.0) {
            return false;
        }
    }

    /* Check c */ {
        const vec3d ca = tri_a - tri_c;
        const vec3d ba = tri_a - tri_b;
        const vec3d cp = p - tri_c;

        vec3d  v = ca - ba * (vec3d::dot(ba, ca) / vec3d::dot(ba, ba));
        double c = 1.0 - vec3d::dot(v, cp) / vec3d::dot(v, ca);

        if(c < 0.0 || c > 1.0) {
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
    vec3d off_a;
    vec3d off_b;
    vec3d off_c;
    vec3d off_d;
};

constexpr static CheckSide sides[6] = {
    /* Y pos */ {
        .side = BlockSide::Y_POS,
        .normal = { 0, 1, 0 },
        .off_a = vec3d{ 0.0, 1.0, 0.0 },
        .off_b = vec3d{ 1.0, 1.0, 0.0 },
        .off_c = vec3d{ 1.0, 1.0, 1.0 },
        .off_d = vec3d{ 0.0, 1.0, 1.0 },
    },
    /* Y neg */ {
        .side = BlockSide::Y_NEG,
        .normal = { 0, -1, 0 },
        .off_a = vec3d{ 0.0, 0.0, 0.0 },
        .off_b = vec3d{ 1.0, 0.0, 0.0 },
        .off_c = vec3d{ 1.0, 0.0, 1.0 },
        .off_d = vec3d{ 0.0, 0.0, 1.0 },
    },
    /* X neg */ {
        .side = BlockSide::X_NEG,
        .normal = { -1, 0, 0 },
        .off_a = vec3d{ 0.0, 0.0, 1.0 },
        .off_b = vec3d{ 0.0, 0.0, 0.0 },
        .off_c = vec3d{ 0.0, 1.0, 0.0 },
        .off_d = vec3d{ 0.0, 1.0, 1.0 },
    },
    /* X pos */ {
        .side = BlockSide::X_POS,
        .normal = { 1, 0, 0 },
        .off_a = vec3d{ 1.0, 0.0, 1.0 },
        .off_b = vec3d{ 1.0, 0.0, 0.0 },
        .off_c = vec3d{ 1.0, 1.0, 0.0 },
        .off_d = vec3d{ 1.0, 1.0, 1.0 },
    },
    /* Z neg */ {
        .side = BlockSide::Z_NEG,
        .normal = { 0, 0, -1 },
        .off_a = vec3d{ 0.0, 0.0, 0.0 },
        .off_b = vec3d{ 1.0, 0.0, 0.0 },
        .off_c = vec3d{ 1.0, 1.0, 0.0 },
        .off_d = vec3d{ 0.0, 1.0, 0.0 },
    },
    /* Z pos */ {
        .side = BlockSide::Z_POS,
        .normal = { 0, 0, 1 },
        .off_a = vec3d{ 0.0, 0.0, 1.0 },
        .off_b = vec3d{ 1.0, 0.0, 1.0 },
        .off_c = vec3d{ 1.0, 1.0, 1.0 },
        .off_d = vec3d{ 0.0, 1.0, 1.0 },
    },
};

RaycastBlockResult raycast_block(World &world, const vec3d &ray_origin, const vec3d &ray_end) {
    RaycastBlockResult result;
    result.found = false;
    result.distance = 100000.0;

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

                BlockType block = chunk->get_block(block_p.block_rel);
                if(block == BlockType::_INVALID || block == BlockType::AIR || get_block_type_flags(block) & IS_NOT_TARGETABLE) {
                    continue;
                }

                for(int32_t index = 0; index < ARRAY_COUNT(sides); ++index) {
                    CheckSide side = sides[index];
                    const vec3d tri_a = block_p.real + side.off_a;
                    const vec3d tri_b = block_p.real + side.off_b;
                    const vec3d tri_c = block_p.real + side.off_c;
                    const vec3d tri_d = block_p.real + side.off_d;

                    if(vec3d::dot(vec3d::normalize(ray_end - ray_origin), vec3d::make(side.normal)) >= 0.0) {
                        continue;
                    }

                    double distance;
                    vec3d  intersection;
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


