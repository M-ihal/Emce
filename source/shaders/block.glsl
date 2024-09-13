@shader_vertex

#version 330 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_tex_coord;

uniform mat4 u_proj;
uniform mat4 u_view;
uniform mat4 u_model;

out vec2 v_tex_coord;
out vec3 v_normal;

void main() {
    v_tex_coord = a_tex_coord;
    v_normal = a_normal;
    gl_Position = u_proj * u_view * u_model * vec4(a_position, 1.0);
}

@shader_fragment

#version 330 core

uniform sampler2D u_texture;

in vec2 v_tex_coord;
in vec3 v_normal;

out vec4 final_color;

void main() {
    const vec3 sun_diffuse = vec3(0.95, 0.95, 0.9);
    const vec3 sun_dir = normalize(vec3(-0.5, -1.0, -0.4));
    vec3 normal        = normalize(v_normal);
    vec3 dir_to_sun    = -sun_dir; 
    float diff = max(dot(normal, dir_to_sun), 0.2);
    vec3  diffuse = sun_diffuse * diff;


    final_color = vec4(diffuse, 1.0) * texture(u_texture, v_tex_coord);
}
