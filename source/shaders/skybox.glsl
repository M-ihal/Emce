@shader_vertex

#version 330 core

layout (location = 0) in vec3 a_position;

uniform mat4 u_proj;
uniform mat4 u_view;

out vec3 v_tex_coord;

void main() {
    /* data must be 1x1 cube */
    v_tex_coord = a_position;

    gl_Position = u_proj * u_view * vec4(a_position, 1.0);
    gl_Position = gl_Position.xyww; // So the depth component is always 1.0 -> z(w)/w
}

@shader_fragment

#version 330 core

uniform samplerCube u_skybox;

in vec3 v_tex_coord;

layout (location = 0) out vec4 out_color;

void main() {
    vec3 tex_coord = v_tex_coord;
    out_color = texture(u_skybox, tex_coord);
}
