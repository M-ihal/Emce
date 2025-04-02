@shader_vertex

#version 330 core

layout (location = 0) in vec3 a_position;

uniform mat4 u_view;
uniform mat4 u_proj;
uniform mat4 u_model;

out vec3 v_position;

void main() {
    v_position = a_position;
    gl_Position = u_proj * u_view * u_model * vec4(a_position, 1.0);
}

@shader_fragment

#version 330 core

uniform vec3  u_color;
uniform vec3  u_size;
uniform float u_width;

/* Percent of u_width */
uniform float u_border_perc  = 0.0;
uniform vec3  u_border_color = vec3(0.0, 0.0, 0.0);

in vec3 v_position;
out vec4 final_color;

void main() {

    float width = u_width;
    float x_perc = width / u_size.x;
    float y_perc = width / u_size.y;
    float z_perc = width / u_size.z;

    bool should_discard = false;
    if(v_position.x > x_perc && v_position.x < 1.0 - x_perc && v_position.y > y_perc && v_position.y < 1.0 - y_perc) should_discard = true;
    if(v_position.z > z_perc && v_position.z < 1.0 - z_perc && v_position.y > y_perc && v_position.y < 1.0 - y_perc) should_discard = true;
    if(v_position.z > z_perc && v_position.z < 1.0 - z_perc && v_position.x > x_perc && v_position.x < 1.0 - x_perc) should_discard = true;

    vec3 norm = normalize(v_position - vec3(0.5, 0.5, 0.5));
    float diff = max(dot(norm, normalize(vec3(0.1, 1.0, -0.1))), 0.4);
    vec3 diffuse = u_color;

    float perc = u_border_perc;

    /* Check if is border */
    if((v_position.x > x_perc * (1.0 - perc) && v_position.x < 1.0 - x_perc * (1.0 - perc)) && v_position.y < (1.0 - y_perc + y_perc * perc) && v_position.y > y_perc - y_perc * perc) {
        diffuse = u_border_color;
    }
    if((v_position.z > z_perc * (1.0 - perc) && v_position.z < 1.0 - z_perc * (1.0 - perc)) && v_position.y < (1.0 - y_perc + y_perc * perc) && v_position.y > y_perc - y_perc * perc) {
        diffuse = u_border_color;
    }
    if((v_position.z > z_perc * (1.0 - perc) && v_position.z < 1.0 - z_perc * (1.0 - perc)) && v_position.x < (1.0 - x_perc + x_perc * perc) && v_position.x > x_perc - x_perc * perc) {
        diffuse = u_border_color;
    }

    diffuse *= diff;

    if(should_discard) {
        discard;
    } else {
        final_color = vec4(diffuse, 1.0);
    }
}

