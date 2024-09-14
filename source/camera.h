#pragma once

#include "common.h"
#include "math_types.h"

class Camera {
public:
    Camera(void);

    void update_free(int32_t move_fw, int32_t move_side, int32_t rotate_v, int32_t rotate_h, float delta_time, bool speed_up);

    vec3 calc_direction(void) const;

    vec3 calc_direction_side(void) const;

    mat4 calc_proj(float aspect_ratio) const;

    mat4 calc_view(void) const;

    void set_position(const vec3 &position);

    vec3 get_position(void) const;

    void set_rotation(const vec2 &rotation);

    vec2 get_rotation(void) const;
   
private:
    vec3  m_position;
    vec2  m_rotation;
    float m_field_of_view;
    float m_plane_near;
    float m_plane_far;
    vec3  m_up_vector;
};
