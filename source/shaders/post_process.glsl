@shader_vertex

#version 330 core

layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_tex_coords;

uniform mat4 u_proj;

out vec2 v_tex_coords;

void main() {
    v_tex_coords = a_tex_coords;
    gl_Position = u_proj * vec4(a_position, 0.0, 1.0);
}

@shader_fragment

#version 330 core

uniform sampler2D u_texture;

uniform int u_in_water = 0;

out vec4 final_color;
in vec2 v_tex_coords;

void main() {
    final_color = texture(u_texture, v_tex_coords);

    if(u_in_water == 1) {
        final_color.r -= 0.25;
        final_color.g -= 0.12;
        final_color.b += 0.20;
    }
}
