#include "camera.h"

#include <math.h>

Camera::Camera(void) {
    m_position = { 0.0f, 0.0f, 0.0f };
    m_rotation = { 0.0f, 0.0f };
    m_plane_near = 0.1f;
    m_plane_far = 100.0f;
    m_field_of_view = DEG_TO_RAD(50.0f); 
    m_up_vector = { 0.0f, 1.0f, 0.0f };
}

/* TODO Pull math to function */
vec3 Camera::calc_direction(void) {
    const float v_angle = m_rotation.y;
    const float h_angle = m_rotation.x;
    return vec3{
        cosf(v_angle) * cosf(h_angle),
        sinf(v_angle),
        cosf(v_angle) * sinf(h_angle)
    };
}

mat4 Camera::calc_proj(float aspect_ratio) {
    return mat4::perspective(m_field_of_view, aspect_ratio, m_plane_near, m_plane_far);
}

mat4 Camera::calc_view(void) {
    vec3 forward = this->calc_direction();
    vec3 focus_p = m_position + forward;
    return mat4::look_at(m_position, focus_p, m_up_vector);
}

void Camera::set_position(const vec3 &position) {
    m_position = position;
}

vec3 Camera::get_position(void) {
    return m_position;
}

void Camera::set_rotation(const vec2 &rotation) {
    m_rotation = rotation;
}

vec2 Camera::get_rotation(void) {
    return m_rotation;
}

void Camera::update_free(int32_t fw_move, int32_t strife, int32_t v_rotate, int32_t h_rotate, float delta_time, bool speed_up) {
}
