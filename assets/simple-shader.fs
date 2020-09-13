#version 330 core

out vec4 o_frag_color;

struct vx_output_t
{
    vec3 color;
};

in vx_output_t v_out;

uniform vec2 u_c;
uniform int u_iterations;
uniform float u_canvassize;

uniform vec3 u_color;
uniform float u_time;

void main()
{
    vec2 xy = v_out.color.yz;
    xy.x -=  0.5 * u_canvassize;//u_canvassize / 2;
    xy.y -=  0.5 * u_canvassize;//u_canvassize / 2;
    o_frag_color = vec4(0.0, 0.3, 0.0, 1.0);
    // o_frag_color = vec4(v_out.color * u_color,1.0);
    // float animation = 0.5 + sin(5 * u_time) * sin(5 * u_time);
    // o_frag_color = vec4(animation * v_out.color * u_color,1.0);

    // vec2 u_c = vec2(-0.8, 0.156);
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
            // o_frag_color = vec4(u_c, 0.0, 1.0);
        }
    }
    if (cutoff < u_iterations) {
        o_frag_color = vec4(1.0, 0.0, 1.0, 1.0) * cutoff / u_iterations;
        // o_frag_color = vec4(u_c, 0.0, 1.0);
    }
}
