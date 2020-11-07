#version 330 core

#define PI 3.1415926538

layout (location = 0) in vec2 in_position;

// struct vx_output_t
// {
//     vec3 normal;
//     vec3 position;
//     vec2 texcoord;
// };
// out vx_output_t v_out;

out vec3 out_normal; // layout (location = 0) 
out vec3 out_position; // layout (location = 1) 
out vec2 out_texcoord; // layout (location = 2) 

uniform sampler2d height_map;
uniform float R;
uniform float r;
uniform float height_mult;


vec3 to_tor(vec3 pos, float R, float r) {
    float long_a = pos.x * 2 * PI;
    float lat_a = pos.y * 2 * PI;

    r += pos.z;
    float radius = R - r * cos(lat_a);
    return vec3(
        cos(long_a) * radius, 
        sin(long_a) * radius, 
        r * sin(lat_a)
    );
}

void main()
{
    out_texcoord = in_position;
    vec3 position = vec3(in_position, texture(height_map, in_position) * height_mult);
    out_position = to_tor(position, R, r);

    out_normal = normalize(to_tor(position + vec3(0, 0, 1), R, r) - position);
}
