#pragma once

#include "common.h"
#include "math_types.h"

class Camera {
public:
    Camera(void);

    void initialize(void);

    void update_free(int32_t move_fw, int32_t move_side, int32_t rotate_v, int32_t rotate_h, double delta_time, bool speed_up);

    void rotate_by(int32_t rotate_v, int32_t rotate_h, double delta_time);

    vec3d calc_direction(void) const;
    vec3d calc_direction_side(void) const;
    vec3d calc_direction_up(void) const;

    vec3d calc_direction_xz(void) const;
    mat4 calc_proj(double aspect_ratio) const;
    mat4 calc_view(void) const;
    mat4 calc_view_at_origin(void) const;

    void set_position(const vec3d &position);
    vec3d get_position(void) const;

    void set_rotation(const vec2d &rotation);
    vec2d get_rotation(void) const;
    
    void  set_fov(double fov);
    double get_fov(void) const;
    vec3d get_up_vector(void) const;
   
    /* Offset given position to be relative to camera, useful when treating camera to be at origin */
    vec3d offset_to_relative(const vec3d &position);

private:
    vec3d  m_position;
    vec2d  m_rotation;      // In radians
    double m_field_of_view; // In radians (fovy)
    double m_plane_near;
    double m_plane_far;
    vec3d  m_up_vector;
};
