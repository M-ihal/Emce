@shader_vertex

#version 330 core

layout (location = 0) in vec3 a_position;

uniform mat4 u_proj;
uniform mat4 u_view;

out vec3 v_tex_coord;

void main() {
    /* Clear translation from view matrix */
    mat4 view_no_move = u_view;
    view_no_move[3][0] = 0.0;
    view_no_move[3][1] = 0.0;
    view_no_move[3][2] = 0.0;

    /* data must be 1x1 cube */
    v_tex_coord = a_position;

    gl_Position = u_proj * view_no_move * vec4(a_position, 1.0);
    gl_Position = gl_Position.xyww; // So the depth component is always 1.0 -> z(w)/w
}

@shader_fragment

#version 330 core

uniform samplerCube u_skybox;

in vec3 v_tex_coord;
out vec4 final_color;

void main() {
    vec3 tex_coord = v_tex_coord;
    //tex_coord.y = -tex_coord.y;
    // :w
    // tex_coord.x = -tex_coord.x;
    final_color = texture(u_skybox, tex_coord);
    // final_color = vec4(v_tex_coord,1.0);
}
