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
out float v_wave_delta;

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

    v_tex_coord = tex_coords[tex_coord_id];

    /* Wave animation */ {
        float wave_speed = 0.4;
        float wave_height = 0.15;
        float wave_frequency_x = 1.0 * 3.14159265 / 32.0 * 2;
        float wave_frequency_z = 2.0 * 3.14159265 / 32.0 * 2;

        float y_base = position.y;

        position.y += wave_height * sin(u_time_elapsed * wave_speed + pos_x * wave_frequency_x) 
            * cos(u_time_elapsed * wave_speed + pos_z * wave_frequency_z);
        
        position.y -= wave_height;


        v_wave_delta = (1.0 - (y_base - position.y + wave_height) / (wave_height * 2.0)) * ((cos(u_time_elapsed) * 0.5f + 0.5f) * 0.7);
        v_wave_delta += (y_base - position.y + wave_height) / (wave_height * 2.0) * ((sin(u_time_elapsed) * 0.5f + 0.5f) * 0.7);
    }
    v_ambient_occlusion = float(ambient_occlusion);


    v_normal = mat3(transpose(inverse(u_model))) * normals[normal_id];
    gl_Position = u_proj * u_view * position;
}

@shader_fragment

#version 330 core

uniform sampler2DArray u_water_texture_array;
uniform samplerCube    u_skybox;

uniform float u_time_elapsed;
uniform int u_load_radius;
uniform int u_fog_enable;
uniform vec3 u_fog_color;

uniform float u_plane_near;
uniform float u_plane_far;

in vec2 v_tex_coord;
in vec3 v_normal;
in float v_ambient_occlusion;
in vec3 v_camera_pos;
in vec3 v_position;
in float v_wave_delta;

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

    vec4 color = texture(u_skybox, frag_to_sky) * vec4(vec3(0.75), 1.0) + vec4(diffuse, 1.0) * texture(u_water_texture_array, tex_arr_coord) * vec4(vec3(0.52), 1.0);
    color *= vec4(1, 1, 1, 0.475);

    float wave_delta = v_wave_delta * v_wave_delta * v_wave_delta;
    color.a += wave_delta * wave_delta * 0.16;
    color.xyz += wave_delta * 0.07;

    float dist_to_frag = length(v_position.xz);

    if(u_fog_enable == 1) {
        /* Fog */ {
            float start = 11 * 32.0;
            float power = 0.8;

            float perc = clamp(dist_to_frag / start - 1.0, 0.0, 1.0) * power;

            perc = perc * perc;
		//perc = sqrt(perc);

            perc = clamp(perc, 0.0, 1.0);

            if(dist_to_frag > start) {
                color.xyz = mix(color.xyz, u_fog_color, perc);
            }
        }

        /* Circle */ {
            float end = float(u_load_radius) * 32.0 - 32.0 - 4.0;
            float start = end - 32.0;
            float perc = clamp((dist_to_frag - start) / (end - start), 0.0, 1.0);

            // perc = 1.0 - sqrt(1.0 - pow(perc, 2));
            perc = perc * perc * perc;

            float diff = clamp(perc, 0.0, 1.0);

            color = mix(color, texture(u_skybox, normalize(v_position)), perc);
        }
    }

    out_color = color;

    out_normal = vec4(v_normal, 1.0);

    // Output linearized depth
    float z_near = u_plane_near;
	float z_far  = u_plane_far;
	float z = gl_FragCoord.z * 2.0 - 1.0;
	float z_final = ((2.0 * z_near * z_far) / (z_far + z_near - z * (z_far - z_near))) / z_far;
	out_depth = vec4(vec3(z_final), 1.0);
}
