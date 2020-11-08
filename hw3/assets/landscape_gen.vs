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

out vec3 out_position;
out vec3 out_normal;
out vec2 out_texcoord;

uniform sampler2D height_map;
uniform sampler2D normal_map;
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
        r * sin(lat_a),
        sin(long_a) * radius
    );
}

mat3 rot(vec3 pos, float R, float r) {
    float long_a = pos.x * 2 * PI;
    float lat_a = pos.y * 2 * PI;

    r += pos.z;
    float radius = R - r * cos(lat_a);

    vec3 gx = vec3(-sin(long_a) * 2 * PI * radius, 0 , cos(long_a) * 2 * PI * radius);
    vec3 gy = vec3(+2 * PI * sin(lat_a) * r * cos(long_a), r * cos(lat_a) * 2 * PI, +2 * PI * sin(lat_a) * r * sin(long_a));
    vec3 gz = vec3(-cos(long_a) * cos(lat_a), sin(lat_a), -sin(long_a) * cos(lat_a));
    
    return mat3(gx, gy, gz);
}

vec3 get_vec(vec2 pos) {
    return vec3(pos, texture(height_map, pos) * height_mult);
}

void main()
{
    out_texcoord = in_position;
    vec3 position = get_vec(in_position);
    out_position = to_tor(position, R, r);

    float eps = 1.0 / 600;
    vec3 dx = get_vec(in_position + vec2(eps, 0)) - get_vec(in_position - vec2(eps, 0));
    vec3 dy = get_vec(in_position + vec2(0, eps)) - get_vec(in_position - vec2(0, eps));
    vec3 grad = normalize(cross(dx, dy));

    // if (dot(grad, vec3(0, 0, 1)) <= 0) {
    //     grad = vec3(0, 0, 1);
    // }

    // vec3 grad = normalize(texture(normal_map, in_position).rgb - 0.5);
    // vec3 grad = vec3(0, 0, 1);
    // grad = grad * 0.1 + vec3(0, 0, 0.9);
    // grad.z *= 5;
    // grad = grad * 0.5 + 0.5;

    out_normal = vec3(0, 0, 0); // actually creating normals on CPU in make_torus(..)
    //out_normal = normalize(to_tor(position + grad * eps, R, r) - to_tor(position - grad * eps, R, r));
    //out_normal = normalize(rot(position, R, r) * grad);
    //out_normal /= sqrt(dot(out_normal, out_normal));
}
