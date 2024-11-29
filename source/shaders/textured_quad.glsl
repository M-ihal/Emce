@shader_vertex

#version 330 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_tex_coords;

uniform mat4 u_proj;
uniform mat4 u_model;

out vec2 v_tex_coords;

void main() {
    v_tex_coords = a_tex_coords;
    gl_Position = u_proj * u_model * vec4(a_position, 1.0);
}

@shader_fragment

#version 330 core

uniform sampler2D u_texture;

in vec2 v_tex_coords;

out vec4 final_color;

void main() {
    final_color = texture(u_texture, v_tex_coords);
}
