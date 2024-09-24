#pragma once

#include "common.h"
#include "math_types.h"

class Player {
public:
    CLASS_COPY_DISABLE(Player);

    Player(void);
    ~Player(void);

    void update(class Game &game, const class Input &input, float delta_time);

    vec3 get_size(void) const;
    void set_position(const vec3 &position);
    vec3 get_position(void) const;
    vec3 get_position_center(void) const;
    vec3 get_position_head(void) const;

private:
    vec3 m_position;
};


