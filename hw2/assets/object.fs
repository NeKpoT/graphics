#version 330 core

out vec4 o_frag_color;

struct vx_output_t
{
    vec3 normal;
    vec3 position;
    vec2 texcoord;
};

in vx_output_t v_out;

uniform samplerCube u_cube;
uniform sampler2D u_tex;

uniform vec3 u_cam;
uniform float u_mirror_a;
uniform float u_prism_a;
uniform float u_prism_n;

void main()
{
    float texture_a = 1 - u_mirror_a - u_prism_a;
    

    vec3 pc = normalize(u_cam - v_out.position);
    vec3 norm = normalize(v_out.normal);

    vec3 pc_sized_norm = norm * dot(pc, norm);
    
    vec3 mirror = texture(u_cube, 2 * pc_sized_norm - pc).rgb;

    vec3 s = pc - pc_sized_norm;
    float cosI = dot(pc, norm);
    float sinT2 = u_prism_n * u_prism_n * (1.0 - cosI * cosI);
    float cosT = sqrt(1 - sinT2);
    // n * cosT + (v - n') * eta
    vec3 prism = texture(u_cube, -(norm * cosT - pc_sized_norm * u_prism_n + pc * u_prism_n)).rgb;
    
    vec3 texture = texture(u_tex, v_out.texcoord).rgb;

    o_frag_color = vec4(mirror * u_mirror_a + prism * u_prism_a + texture * texture_a, 1.0);
}
