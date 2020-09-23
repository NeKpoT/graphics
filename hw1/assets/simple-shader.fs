#version 330 core

out vec4 o_frag_color;

struct vx_output_t
{
    vec2 position;
};
in vx_output_t v_out;

uniform vec2 u_c;
uniform int u_iterations;
uniform float u_canvassize;

uniform vec3 u_color;
uniform float u_time;

uniform sampler1D u_tex;

void main()
{
    vec2 xy = v_out.position;
    // xy.x -=  0.5 * u_canvassize;
    // xy.y -=  0.5 * u_canvassize;
    o_frag_color = vec4(0.0, 0.0, 0.0, 1.0);

    float R = 0.5 + sqrt(1 + 4*length(u_c)) / 2;
    vec2 xy2;
    int cutoff = u_iterations;
    for (int i = 0; i < u_iterations; i++) {
        xy2.x = xy.x * xy.x - xy.y * xy.y + u_c.x;
        xy2.y = 2 * xy.x * xy.y + u_c.y;
        xy = xy2;
        if (length(xy) > R) {
            cutoff = i;
            break;
        }
    }
    if (cutoff < u_iterations) {
        // o_frag_color = vec4(1.0, 0.0, 1.0, 1.0) * cutoff / u_iterations;
        vec3 texture = texture(u_tex, cutoff * 1.0 / u_iterations * 0.99 + 0.01).rgb;
        o_frag_color = vec4(texture, 1.0);
    }
}
