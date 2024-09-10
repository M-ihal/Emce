#pragma once

#include "common.h"
#include "math_types.h"

class Camera {
public:
    Camera(void);

    void update_free(int32_t fw_move, int32_t strife, int32_t v_rotate, int32_t h_rotate, float delta_time, bool speed_up);

    vec3 calc_direction(void);

    mat4 calc_proj(float aspect_ratio);

    mat4 calc_view(void);

    void set_position(const vec3 &position);

    vec3 get_position(void);

    void set_rotation(const vec2 &rotation);

    vec2 get_rotation(void);
   
private:
    vec3  m_position;
    vec2  m_rotation;
    float m_field_of_view;
    float m_plane_near;
    float m_plane_far;
    vec3  m_up_vector;
};
