@shader_vertex

#version 330 core

vec2 tex_coords[4] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 1.0)
);

vec3 normals[6] = vec3[](
    vec3(-1.0, 0.0, 0.0),
    vec3( 1.0, 0.0, 0.0),
    vec3( 0.0, 0.0,-1.0),
    vec3( 0.0, 0.0, 1.0),
    vec3( 0.0,-1.0, 0.0),
    vec3( 0.0, 1.0, 0.0)
);

layout (location = 0) in uvec2 a_packed;

uniform mat4 u_proj;
uniform mat4 u_view;
uniform mat4 u_model;

uniform float u_time_elapsed;

out vec2 v_tex_coord;
out vec3 v_normal;
out float v_ambient_occlusion;
out vec3 v_camera_pos;
out vec3 v_position;

void main() {
    uint pos_x = (a_packed.x >> 0u) & 0xFFu;
    uint pos_y = (a_packed.x >> 8u) & 0xFFu;
    uint pos_z = (a_packed.x >> 16u) & 0xFFu;
    uint normal_id = (a_packed.x >> 24u) & 0xFu;

    // uint tex_slot = (a_packed.y >> 0u) & 0xFFu;
    uint tex_coord_id = (a_packed.y >> 8u) & 0x3u;
    uint ambient_occlusion = (a_packed.y >> 10u) & 0x3u;

    mat4 inv_view = inverse(u_view);
    v_camera_pos = inv_view[3].xyz;

    vec4 position = u_model * vec4(pos_x, pos_y, pos_z, 1.0);

    v_position = (u_model * vec4(pos_x, pos_y, pos_z, 1.0)).xyz;

    /* Wave animation */ {
        float wave_speed = 1.0;
        float wave_height = 0.22;
        float wave_frequency = 2.0 * 3.14159265 / 32.0 * 2;

        position.y += wave_height * sin(u_time_elapsed * wave_speed + position.x * wave_frequency) 
            * cos(u_time_elapsed * wave_speed + position.z * wave_frequency);
        
        position.y -= 0.2;
    }
    v_ambient_occlusion = float(ambient_occlusion);


    v_tex_coord = tex_coords[tex_coord_id];
    v_normal = mat3(transpose(inverse(u_model))) * normals[normal_id];
    gl_Position = u_proj * u_view * position;
}

@shader_fragment

#version 330 core

uniform sampler2DArray u_water_texture_array;
uniform samplerCube    u_skybox;

uniform float u_time_elapsed;

in vec2 v_tex_coord;
in vec3 v_normal;
in float v_ambient_occlusion;
in vec3 v_camera_pos;
in vec3 v_position;

layout (location = 0) out vec4 out_color;
layout (location = 1) out vec4 out_normal;
layout (location = 2) out vec4 out_depth;

void main() {
    float ambient_occlusion = mix(0.3, 1.0, v_ambient_occlusion / 3.0);

    const vec3 sun_diffuse = vec3(0.8, 0.82, 0.85);
    const vec3 sun_dir = normalize(vec3(-0.3, -0.55, +0.22));
    vec3 normal        = normalize(v_normal);
    vec3 dir_to_sun    = -sun_dir; 
    float diff = max(dot(normal, dir_to_sun), 0.35);
    vec3  diffuse = sun_diffuse * diff * ambient_occlusion;

    int water_frame = int(u_time_elapsed * 30.0) % 32;
    vec3 tex_arr_coord = vec3(v_tex_coord, float(water_frame));

    vec3 camera_to_frag = normalize(vec3(v_position - v_camera_pos));
    vec3 frag_to_sky = reflect(camera_to_frag, normal);

 //   out_color = texture(u_water_texture_array, tex_arr_coord) * vec4(1,1,1,0.8);
    out_color = texture(u_skybox, frag_to_sky) + vec4(diffuse, 1.0) * texture(u_water_texture_array, tex_arr_coord) * 0.22;
    out_color *= vec4(1, 1, 1, 0.5);

    out_normal = vec4(v_normal, 1.0);

    // final_color = vec4(0, 0, 0, 1);
    
    // @todo hardcoded
    float z_near = 0.1;
	float z_far  = 300.0;
	float z = gl_FragCoord.z * 2.0 - 1.0;
	float z_final = ((2.0 * z_near * z_far) / (z_far + z_near - z * (z_far - z_near))) / z_far;
	out_depth = vec4(vec3(z_final), 1.0);
}
