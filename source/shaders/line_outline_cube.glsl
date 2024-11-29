@shader_vertex

#version 330 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;

uniform mat4 u_view;
uniform mat4 u_proj;
uniform mat4 u_model;
uniform vec3 u_direction;

out vec3 v_position;
out bool v_discard;

void main() {
    v_discard = dot(normalize(u_direction), normalize(a_normal)) > 0.5;

    v_position = a_position;
    //gl_Position = u_proj * u_view * u_model * vec4(a_position, 1.0);
}

@shader_fragment

#version 330 core

uniform vec3  u_color;

in bool v_discard;
in vec3 v_position;
out vec4 final_color;

void main() {
    discard;

    if(v_discard) {
        discard;
    } else {
        final_color = vec4(1.0, 0.0, 0.0, 1.0);
    }
}

