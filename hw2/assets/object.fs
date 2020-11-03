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
uniform float u_texture_a;
uniform float u_prism_n;
uniform bool u_tex_gamma_correct;
uniform bool u_blend_gamma_correct;

const float GAMMA = 2.2;
const float UNGAMMA = 1 / GAMMA;

vec3 gamma(vec3 v) {
    return pow(v, vec3(GAMMA, GAMMA, GAMMA));
}

vec3 ungamma(vec3 v) {
    return pow(v, vec3(UNGAMMA, UNGAMMA, UNGAMMA));
}

void main()
{
    float optics_a = 1 - u_texture_a;
    

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

    float r_ref = (cosI * u_prism_n - cosT) / (cosI * u_prism_n + cosT);
    r_ref = r_ref * r_ref;
    float r_prism = 1 - r_ref;
    
    vec3 texture = texture(u_tex, v_out.texcoord).rgb;

    if (u_tex_gamma_correct) {
        texture = gamma(texture);
    }

    vec3 color_out;

    if (u_blend_gamma_correct) {
        mirror = ungamma(mirror);
        prism = ungamma(prism);
        texture = ungamma(texture);
    }
    color_out = (mirror * r_ref + prism * r_prism) * (1 - u_texture_a) + texture * u_texture_a;
    if (u_blend_gamma_correct) {
        color_out = gamma(color_out);
    }
    

    o_frag_color = vec4(color_out, 1.0);
    // o_frag_color = vec4(r_prism * vec3(1, 1, 1), 1);
}
