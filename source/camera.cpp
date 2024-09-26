#include "camera.h"

#include <math.h>

Camera::Camera(void) {
    m_position = { 0.0f, 0.0f, 0.0f };
    m_rotation = { 0.0f, 0.0f };
    m_plane_near = 0.1f;
    m_plane_far = 1000.0f;
    m_field_of_view = DEG_TO_RAD(55.0f); 
    m_up_vector = { 0.0f, 1.0f, 0.0f };
}

/* TODO Pull math to function */
vec3 Camera::calc_direction(void) const {
    const float v_angle = m_rotation.y;
    const float h_angle = m_rotation.x;
    return vec3{
        cosf(v_angle) * cosf(h_angle),
        sinf(v_angle),
        cosf(v_angle) * sinf(h_angle)
    };
}

vec3 Camera::calc_direction_side(void) const {
    const float v_angle = 0.0f;
    const float h_angle = m_rotation.x + M_PI * 0.5f;
    return vec3{
        cosf(v_angle) * cosf(h_angle),
        sinf(v_angle),
        cosf(v_angle) * sinf(h_angle)
    };
}

vec3 Camera::calc_direction_xz(void) const {
    const float v_angle = 0.0f;
    const float h_angle = m_rotation.x;
    return vec3{
        cosf(v_angle) * cosf(h_angle),
        sinf(v_angle),
        cosf(v_angle) * sinf(h_angle)
    };
}

mat4 Camera::calc_proj(float aspect_ratio) const {
    return mat4::perspective(m_field_of_view, aspect_ratio, m_plane_near, m_plane_far);
}

mat4 Camera::calc_view(void) const {
    vec3 forward = this->calc_direction();
    vec3 focus_p = m_position + forward;
    return mat4::look_at(m_position, focus_p, m_up_vector);
}

void Camera::set_position(const vec3 &position) {
    m_position = position;
}

vec3 Camera::get_position(void) const {
    return m_position;
}

void Camera::set_rotation(const vec2 &rotation) {
    m_rotation = rotation;
}

vec2 Camera::get_rotation(void) const {
    return m_rotation;
}

void Camera::set_fov(float fov) {
    ASSERT(fov > 0.0f);
    m_field_of_view = fov;
}

float Camera::get_fov(void) const {
    return m_field_of_view;
}

void Camera::update_free(int32_t move_fw, int32_t move_side, int32_t rotate_v, int32_t rotate_h, float delta_time, bool speed_up) {
    constexpr float SPEED_MOVE_FAST = 40.0f;
    constexpr float SPEED_MOVE = 0.5f;

    this->rotate_by(rotate_v, rotate_h, delta_time);

    const float now_speed = speed_up ? SPEED_MOVE_FAST : SPEED_MOVE;

    /* Move forwards or backwards */
    vec3 dir_fw = this->calc_direction();
    m_position.x += move_fw * dir_fw.x * now_speed * delta_time;
    m_position.y += move_fw * dir_fw.y * now_speed * delta_time;
    m_position.z += move_fw * dir_fw.z * now_speed * delta_time;

    /* Move left or right */
    vec3 dir_side = this->calc_direction_side();
    m_position.x += move_side * dir_side.x * now_speed * delta_time;
    m_position.z += move_side * dir_side.z * now_speed * delta_time;
}

void Camera::rotate_by(int32_t rotate_v, int32_t rotate_h, float delta_time) {
    constexpr float SPEED_ROTATE = 0.0075f;

    m_rotation.x += rotate_h * SPEED_ROTATE;
    m_rotation.y += rotate_v * SPEED_ROTATE;

    const float EPS = 0.01f;
    m_rotation.y = clamp(m_rotation.y, DEG_TO_RAD(-90.0f + EPS), DEG_TO_RAD(90.0f - EPS));
    m_rotation.x = wrap(m_rotation.x, 0.0f, M_PI * 2.0f);
}
