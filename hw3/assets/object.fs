#version 330 core

out vec4 o_frag_color;

struct vx_output_t
{
    vec3 normal;
    vec3 position;
    vec2 texcoord;
    float h;
};

in vx_output_t v_out;

uniform samplerCube u_cube;

uniform sampler2D u_tex0;
uniform sampler2D u_tex1;
uniform float u_texture_a0;
uniform float u_texture_a1;
uniform float u_prism_n0;
uniform float u_prism_n1;
uniform vec3 u_color0;
uniform vec3 u_color1;

uniform vec3 u_cam;

uniform bool u_tex_gamma_correct;
uniform bool u_blend_gamma_correct;

uniform float u_badrock_height;

const float GAMMA = 2.2;
const float UNGAMMA = 1 / GAMMA;

vec3 gamma(vec3 v) {
    return pow(v, vec3(GAMMA, GAMMA, GAMMA));
}

vec3 ungamma(vec3 v) {
    return pow(v, vec3(UNGAMMA, UNGAMMA, UNGAMMA));
}

const float FULL_DETAIL_DIST = 2;
const float NO_DETAIL_DIST = 4;

vec4 get_texture(sampler2D tex, vec2 pos) {
    vec4 tex_v = texture(tex, pos);
    if (tex_v == vec4(0, 0, 0, 0)) {
        tex_v = vec4(u_color0, 0);
    }
    // float dist = distance(u_cam, v_out.position);
    // if (dist < NO_DETAIL_DIST) {
    //     float detail_a = max(0, (NO_DETAIL_DIST - dist) / (NO_DETAIL_DIST - FULL_DETAIL_DIST)) / 4;
    //     tex_v = tex_v * (1 - detail_a) + texture(tex, pos * 10) * detail_a;
    // }
    return tex_v;
}

vec3 color(sampler2D u_tex, float u_texture_a, float u_prism_n)
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
    
    vec3 tex = get_texture(u_tex, v_out.texcoord).rgb;

    if (u_tex_gamma_correct) {
        tex = gamma(tex);
    }

    vec3 color_out;

    if (u_blend_gamma_correct) {
        mirror = ungamma(mirror);
        prism = ungamma(prism);
        tex = ungamma(tex);
    }
    color_out = (mirror * r_ref + prism * r_prism) * (1 - u_texture_a) + tex * u_texture_a;
    if (u_blend_gamma_correct) {
        color_out = gamma(color_out);
    }
    

    return color_out;
    // o_frag_color = texture(u_cube, -norm); // see-through
    // o_frag_color = vec4((norm / 2 + 0.5).r, 0.0, 0.0, 1.0); // show norm
    // o_frag_color = vec4(r_prism * vec3(1, 1, 1), 1); // show prism
}

void main() {
    o_frag_color = vec4(color(u_tex0, u_texture_a0, u_prism_n0), 1);
}