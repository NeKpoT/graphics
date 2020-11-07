#version 330 core

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;

struct vx_output_t
{
    vec3 normal;
    vec3 position;
    vec2 texcoord;
};
out vx_output_t v_out;

uniform mat4 u_mvp;
uniform vec3 u_cam;

void main()
{
    v_out.normal = normal;
    v_out.position = in_position;
    v_out.texcoord = texcoord;
    gl_Position = u_mvp * vec4(in_position, 1.0);
}
