#include "world_utils.h"
#include "world.h"

bool is_chunk_in_range(vec2i chunk_xz, vec2i origin, int32_t radius) {
    bool result = chunk_xz.x >= origin.x - radius
        && chunk_xz.x <= origin.x + radius
        && chunk_xz.y >= origin.y - radius
        && chunk_xz.y <= origin.y + radius;
    return result;
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
                if(!block || block->type == BlockType::AIR || get_block_flags(block->type) & IS_NOT_TARGETABLE) {
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

