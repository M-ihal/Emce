@shader_vertex

#version 330 core

layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_tex_coord;

uniform mat4 u_proj;

out vec2 v_tex_coord;

void main() {
    v_tex_coord = a_tex_coord;
    gl_Position = u_proj * vec4(a_position, 0.0, 1.0);
}

@shader_fragment

#version 330 core

uniform vec3 u_color;
uniform sampler2D u_font_atlas;

in  vec2 v_tex_coord;
out vec4 final_color;

void main() {
    float alpha = texture(u_font_atlas, v_tex_coord).r;
    final_color = vec4(u_color, alpha);
    if(final_color.a < 0.1) {
        discard;
    }
}
