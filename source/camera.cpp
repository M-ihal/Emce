#include "camera.h"

#include <math.h>

Camera::Camera(void) {
    this->initialize();
}

void Camera::initialize(void) {
    m_position = { 0.0f, 0.0f, 0.0f };
    m_rotation = { 0.0f, 0.0f };
    m_plane_near = 0.1f;
    m_plane_far = 2000.0f;
    m_field_of_view = DEG_TO_RAD(70.0f); 
    m_up_vector = { 0.0f, 1.0f, 0.0f };
}

vec3d Camera::calc_direction(void) const {
    const double v_angle = m_rotation.y;
    const double h_angle = m_rotation.x;
    return vec3d{
        cosf(v_angle) * cosf(h_angle),
        sinf(v_angle),
        cosf(v_angle) * sinf(h_angle)
    };
}

vec3d Camera::calc_direction_side(void) const {
    const double v_angle = 0.0f;
    const double h_angle = m_rotation.x + M_PI * 0.5f;
    return vec3d{
        cosf(v_angle) * cosf(h_angle),
        sinf(v_angle),
        cosf(v_angle) * sinf(h_angle)
    };
}

vec3d Camera::calc_direction_up(void) const {
    const double v_angle = m_rotation.y + M_PI * 0.5f;
    const double h_angle = m_rotation.x;
    return vec3d{
        cosf(v_angle) * cosf(h_angle),
        sinf(v_angle),
        cosf(v_angle) * sinf(h_angle)
    };
}

vec3d Camera::get_up_vector(void) const {
    return m_up_vector;
}

vec3d Camera::calc_direction_xz(void) const {
    const double v_angle = 0.0f;
    const double h_angle = m_rotation.x;
    return vec3d{
        cosf(v_angle) * cosf(h_angle),
        sinf(v_angle),
        cosf(v_angle) * sinf(h_angle)
    };
}

mat4 Camera::calc_proj(double aspect_ratio) const {
    return mat4::perspective(m_field_of_view, aspect_ratio, m_plane_near, m_plane_far);
}

mat4 Camera::calc_view(void) const {
    vec3d position = m_position;
    vec3d forward = this->calc_direction();
    vec3d focus_p = position + forward;
    return mat4::look_at(position, focus_p, m_up_vector);
}

mat4 Camera::calc_view_at_origin(void) const {
    vec3d forward = this->calc_direction();
    return mat4::look_at(vec3d::zero(), forward, m_up_vector);
}

void Camera::set_position(const vec3d &position) {
    m_position = position;
}

vec3d Camera::get_position(void) const {
    return m_position;
}

void Camera::set_rotation(const vec2d &rotation) {
    m_rotation = rotation;
}

vec2d Camera::get_rotation(void) const {
    return m_rotation;
}

void Camera::set_fov(double fov) {
    ASSERT(fov > 0.0f);
    m_field_of_view = fov;
}

double Camera::get_fov(void) const {
    return m_field_of_view;
}

vec3d Camera::offset_to_relative(const vec3d &position) {
    vec3d relative = position - m_position;
    return relative;
}

void Camera::update_free(int32_t move_fw, int32_t move_side, int32_t rotate_v, int32_t rotate_h, double delta_time, bool speed_up) {
    // Pass as arguments @todo
    constexpr double SPEED_MOVE_FAST = 40.0f;
    constexpr double SPEED_MOVE = 0.5f;

    this->rotate_by(rotate_v, rotate_h, delta_time);

    const double now_speed = speed_up ? SPEED_MOVE_FAST : SPEED_MOVE;

    /* Move forwards or backwards */
    vec3d dir_fw = this->calc_direction();
    m_position.x += move_fw * dir_fw.x * now_speed * delta_time;
    m_position.y += move_fw * dir_fw.y * now_speed * delta_time;
    m_position.z += move_fw * dir_fw.z * now_speed * delta_time;

    /* Move left or right */
    vec3d dir_side = this->calc_direction_side();
    m_position.x += move_side * dir_side.x * now_speed * delta_time;
    m_position.z += move_side * dir_side.z * now_speed * delta_time;
}

void Camera::rotate_by(int32_t rotate_v, int32_t rotate_h, double delta_time) {
    // Pass as argument @todo
    constexpr double SPEED_ROTATE = 0.0035f;

    m_rotation.x += rotate_h * SPEED_ROTATE;
    m_rotation.y += rotate_v * SPEED_ROTATE;

    const double EPS = 5.0f;
    m_rotation.y = clamp(m_rotation.y, (double)DEG_TO_RAD(-90.0f + EPS), (double)DEG_TO_RAD(90.0f - EPS));
    m_rotation.x = wrap(m_rotation.x, 0.0f, M_PI * 2.0f);
}
