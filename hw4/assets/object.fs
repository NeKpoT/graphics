#version 330 core

#myinclude_light

out vec4 o_frag_color;

struct vx_output_t
{
    vec3 normal;
    vec3 position;
    vec2 texcoord;
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
    // if (tex_v == vec4(0, 0, 0, 0)) {
    //     tex_v = vec4(u_color0, 0);
    // }
    return tex_v;
}

vec3 color(sampler2D u_tex, float u_texture_a)
{
    float optics_a = 1 - u_texture_a;
    

    vec3 pc = normalize(u_cam - v_out.position);
    vec3 norm = normalize(v_out.normal);

    vec3 pc_sized_norm = norm * dot(pc, norm);
    
    vec3 mirror = texture_cubemap(u_cube, 2 * pc_sized_norm - pc).rgb;

    vec3 tex = get_texture(u_tex0, v_out.texcoord).rgb;
    // tex = ungamma(tex);
    tex = get_light(v_out.position, u_cam, v_out.normal, tex, 0.8);

    if (u_tex_gamma_correct) {
        tex = gamma(tex);
    }

    vec3 color_out;

    if (u_blend_gamma_correct) {
        mirror = ungamma(mirror);
        tex = ungamma(tex);
    }
    color_out = mirror * optics_a + tex * u_texture_a;
    if (u_blend_gamma_correct) {
        color_out = gamma(color_out);
    }
    

    return color_out;
    // return tex; // show norm
    // o_frag_color = texture_cubemap(u_cube, -norm); // see-through
    // o_frag_color = vec4(r_prism * vec3(1, 1, 1), 1); // show prism
}

void main() {
    o_frag_color = vec4(color(u_tex0, u_texture_a0), 1);
}