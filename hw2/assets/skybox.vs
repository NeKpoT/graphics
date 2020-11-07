#version 330 core

layout (location = 0) in vec3 in_position;

struct vx_output_t
{
    vec3 world_position;
};
out vx_output_t v_out;

uniform mat4 u_mvp;

void main()
{
    v_out.world_position = in_position;
    gl_Position = u_mvp * vec4(in_position, 1.0);
}
