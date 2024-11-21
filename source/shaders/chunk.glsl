@shader_vertex

#version 330 core

vec2 tex_coords[4] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 1.0)
);

vec3 normals[] = vec3[](
    vec3(-1.0, 0.0, 0.0),
    vec3( 1.0, 0.0, 0.0),
    vec3( 0.0, 0.0,-1.0),
    vec3( 0.0, 0.0, 1.0),
    vec3( 0.0,-1.0, 0.0),
    vec3( 0.0, 1.0, 0.0),
    vec3( 0.707107, 0.0,  0.707107),
    vec3(-0.707107, 0.0,  0.707107),
    vec3(-0.707107, 0.0, -0.707107),
    vec3( 0.707107, 0.0, -0.707107)
);

layout (location = 0) in uvec2 a_packed;

uniform mat4 u_proj;
uniform mat4 u_view;
uniform mat4 u_model;

out vec2 v_tex_coord;
out vec3 v_normal;
out float v_tex_slot;

out vec3 v_pos;

void main() {
    uint pos_x = (a_packed.x >> 0u) & 0xFFu;
    uint pos_y = (a_packed.x >> 8u) & 0xFFu;
    uint pos_z = (a_packed.x >> 16u) & 0xFFu;
    uint normal_id = (a_packed.x >> 24u) & 0xFu;

    uint tex_slot = (a_packed.y >> 0u) & 0xFFu;
    uint tex_coord_id = (a_packed.y >> 8u) & 0x3u;

    v_tex_coord = tex_coords[tex_coord_id];
    v_normal = mat3(transpose(inverse(u_model))) * normals[normal_id];
    v_tex_slot = tex_slot;

    v_pos = vec3(float(pos_x + 0.5), float(pos_y), float(pos_z));
    gl_Position = u_proj * u_view * u_model * vec4(pos_x, pos_y, pos_z, 1.0);
}

@shader_fragment

#version 330 core

uniform sampler2DArray u_texture_array;

in vec2  v_tex_coord;
in vec3  v_normal;
in float v_tex_slot;

in vec3 v_pos;

layout (location = 0) out vec4 out_color;
layout (location = 1) out vec4 out_normal;
layout (location = 2) out vec4 out_depth;

void main() {
    const vec3 sun_diffuse = vec3(0.8, 0.82, 0.85);
    const vec3 sun_dir = normalize(vec3(-0.3, -0.55, +0.22));
    vec3 normal        = normalize(v_normal);
    vec3 dir_to_sun    = -sun_dir; 
    float diff = max(dot(normal, dir_to_sun), 0.35);
    vec3  diffuse = sun_diffuse * diff;

    // vertex sh
    vec3 tex_arr_coord = vec3(v_tex_coord, floor(v_tex_slot + 0.5));

    if(texture(u_texture_array, tex_arr_coord).a < 0.1){
        discard;
    }

    out_color = vec4(diffuse, 1.0) * texture(u_texture_array, tex_arr_coord);

    out_normal = vec4(v_normal, 1.0);


    // @todo hardcoded
    float z_near = 0.1;
	float z_far  = 300.0;
	float z = gl_FragCoord.z * 2.0 - 1.0;
	float z_final = ((2.0 * z_near * z_far) / (z_far + z_near - z * (z_far - z_near))) / z_far;
	out_depth = vec4(vec3(z_final), 1.0);
}
