#version 330 core

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_coord;

struct vx_output_t
{
    vec2 position;
};
out vx_output_t v_out;

uniform float u_zoom;
uniform vec2 u_translation;

void main()
{
    vec2 zoomed_pos;
    zoomed_pos.x = u_translation.x + (in_coord.y - 0.5) * (1 / u_zoom);
    zoomed_pos.y = u_translation.y + (in_coord.z - 0.5) * (1 / u_zoom);

    v_out.position = zoomed_pos;

    gl_Position = vec4(in_position.x, in_position.y, in_position.z, 1.0);
}
