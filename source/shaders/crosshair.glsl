@shader_vertex

#version 330 core

layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_coords;

uniform mat4 u_proj;
uniform mat4 u_model;

out vec2 v_coords;

void main() {
    v_coords = a_coords;
    gl_Position = u_proj * u_model * vec4(a_position, 0.0, 1.0);
}

@shader_fragment

#version 330 core

uniform float u_perc;

in vec2 v_coords;
out vec4 frag_color;

void main() {
    float perc = 1.0 - u_perc;
    float x_perc = (0.5 - abs(v_coords.x - 0.5)) * 2.0;
    float y_perc = (0.5 - abs(v_coords.y - 0.5)) * 2.0;

    float alpha = 0.5;
    if(x_perc < perc && y_perc < perc) {
        alpha = 0.0;
    }
    frag_color = vec4(1.0, 1.0, 1.0, alpha);
}
