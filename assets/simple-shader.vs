#version 330 core

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_color;

struct vx_output_t
{
    vec3 color;
};
out vx_output_t v_out;

uniform float u_zoom;
uniform vec2 u_translation;

void main()
{
    vec2 rotated_pos;
    rotated_pos.x = (u_translation.x + in_position.x) * u_zoom;
    rotated_pos.y = (u_translation.y + in_position.y) * u_zoom;

    v_out.color = in_color;
    //v_out.color.x = 0;

    gl_Position = vec4(rotated_pos.x, rotated_pos.y, in_position.z, 1.0);
}
