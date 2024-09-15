#include "world.h"

World::World(void) {
    for(int32_t x = 0; x < 10; ++x) {
        for(int32_t z = 0; z < 10; ++z) {
            m_chunks.push_back(std::make_tuple(new Chunk(), vec3i{ x, 0, z }));
        }
    }
}

World::~World(void) {

}

void World::render_chunks(const Shader &shader, const Texture &sand_texture) {
    for(const std::tuple<Chunk *, vec3i> &chunk : m_chunks) {
        std::get<0>(chunk)->render(std::get<1>(chunk), shader, sand_texture);
    }
}
