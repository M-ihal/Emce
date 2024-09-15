@shader_vertex

#version 330 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_tex_coord;

uniform mat4 u_proj;
uniform mat4 u_view;
uniform mat4 u_model;

out vec2 v_tex_coord;

void main() {
    v_tex_coord = a_tex_coord;
    gl_Position = u_proj * u_view * u_model * vec4(a_position, 1.0);
}

@shader_fragment

#version 330 core

in vec2 v_tex_coord;
out vec4 f_color;

uniform sampler2D u_texture;

void main() {
    // f_color = vec4(v_color, 1.0);
    f_color = texture(u_texture, v_tex_coord);
    f_color = vec4(1,0,1,1);
}

