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
uniform int u_load_radius;

out vec3 v_position;
out vec2 v_tex_coord;
out vec3 v_normal;
out float v_tex_slot;
out float v_ambient_occlusion;
out float v_fog_factor;

void main() {
    uint pos_x = (a_packed.x >> 0u) & 0xFFu;
    uint pos_y = (a_packed.x >> 8u) & 0xFFu;
    uint pos_z = (a_packed.x >> 16u) & 0xFFu;
    uint normal_id = (a_packed.x >> 24u) & 0xFu;
    uint tex_slot = (a_packed.y >> 0u) & 0xFFu;
    uint tex_coord_id = (a_packed.y >> 8u) & 0x3u;
    uint ambient_occlusion = (a_packed.y >> 10u) & 0x3u;

    vec3 position = (u_model * vec4(pos_x, pos_y, pos_z, 1.0)).xyz;

    v_tex_coord = tex_coords[tex_coord_id];
    v_normal = mat3(transpose(inverse(u_model))) * normals[normal_id];
    v_tex_slot = tex_slot;
    v_ambient_occlusion = float(ambient_occlusion / 3.0);
    v_position = position;
    v_fog_factor = 0.0;
    gl_Position = u_proj * u_view * u_model * vec4(pos_x, pos_y, pos_z, 1.0);
}

@shader_fragment

#version 330 core

uniform sampler2DArray u_texture_array;
uniform samplerCube u_skybox;

uniform float u_plane_near;
uniform float u_plane_far;

// For fog
uniform int u_load_radius;
uniform int u_fog_enable;
uniform vec3 u_fog_color;

in vec3  v_position;
in vec2  v_tex_coord;
in vec3  v_normal;
in float v_tex_slot;
in float v_ambient_occlusion;
in float v_fog_factor;

layout (location = 0) out vec4 out_color;
layout (location = 1) out vec4 out_normal;
layout (location = 2) out vec4 out_depth;
layout (location = 3) out vec4 out_ambient_occlusion;

void main() {
    float ambient_occlusion = max(0.2, v_ambient_occlusion);

    const vec3 sun_diffuse = vec3(0.92, 0.95, 0.94) * vec3(1.02);
    const vec3 sun_dir = normalize(vec3(+0.4, -0.75, -0.12));
    vec3 normal = normalize(v_normal);
    vec3 dir_to_sun = -sun_dir; 
    float diff = max(dot(normal, dir_to_sun), 0.6);
    vec3 diffuse = sun_diffuse * diff * ambient_occlusion;

    // vertex sh
    vec3 tex_arr_coord = vec3(v_tex_coord, floor(v_tex_slot + 0.5));

    if(texture(u_texture_array, tex_arr_coord).a < 0.1){
        discard;
    }

    vec4 color = vec4(diffuse, 1.0) * texture(u_texture_array, tex_arr_coord);

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
    out_ambient_occlusion = vec4(vec3(ambient_occlusion), 1.0);

    // Output linearized depth
    float z_near = u_plane_near;
	float z_far  = u_plane_far;
	float z = gl_FragCoord.z * 2.0 - 1.0;
	float z_final = ((2.0 * z_near * z_far) / (z_far + z_near - z * (z_far - z_near))) / z_far;
	out_depth = vec4(vec3(z_final), 1.0);
}
