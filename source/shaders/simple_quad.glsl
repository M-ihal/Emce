@shader_vertex

#version 330 core

layout (location = 0) in vec2 a_position;

uniform mat4 u_proj;
uniform mat4 u_model;

void main() {
    gl_Position = u_proj * u_model * vec4(a_position, 0.0, 1.0);
}

@shader_fragment

#version 330 core

uniform vec4 u_color;

out vec4 final_color;

void main() {
    final_color = u_color;
}
