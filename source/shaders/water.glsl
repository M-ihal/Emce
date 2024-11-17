@shader_vertex

#version 330 core

// @todo make water faces only facing upwards ??

layout (location = 0) in vec3  a_position;
layout (location = 1) in vec3  a_normal;
layout (location = 2) in vec2  a_tex_coord;
layout (location = 3) in float a_tex_slot;

uniform mat4 u_proj;
uniform mat4 u_view;
uniform mat4 u_model;

uniform float u_time_elapsed;

out vec2 v_tex_coord;
out vec3 v_normal;
out float v_water_frame;

void main() {
    vec4 position = u_model * vec4(a_position, 1.0);

    /* Wave animation */ {
        float wave_speed = 0.8;
        float wave_height = 0.18;
        float wave_frequency = 32.0;

        position.y += wave_height * sin(u_time_elapsed * wave_speed + position.x * wave_frequency) 
            * cos(u_time_elapsed * wave_speed + position.z * wave_frequency);
        
        position.y -= 0.2;
    }

    v_tex_coord = a_tex_coord;
    v_normal = mat3(transpose(inverse(u_model))) * a_normal;
    v_water_frame = a_tex_slot;
    gl_Position = u_proj * u_view * position;
}

@shader_fragment

#version 330 core

uniform sampler2DArray u_water_texture_array;

uniform float u_time_elapsed;

in vec2 v_tex_coord;
in vec3 v_normal;

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

    int water_frame = int(u_time_elapsed * 30.0) % 32;
    vec3 tex_arr_coord = vec3(v_tex_coord, float(water_frame));

    out_color = vec4(diffuse, 1.0) * texture(u_water_texture_array, tex_arr_coord);

    out_normal = vec4(v_normal, 1.0);

    // final_color = vec4(0, 0, 0, 1);
    
    // @todo hardcoded
    float z_near = 0.1;
	float z_far  = 300.0;
	float z = gl_FragCoord.z * 2.0 - 1.0;
	float z_final = ((2.0 * z_near * z_far) / (z_far + z_near - z * (z_far - z_near))) / z_far;
	out_depth = vec4(vec3(z_final), 1.0);
}
