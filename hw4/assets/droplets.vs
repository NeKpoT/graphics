#version 330 core

layout (location = 0) in vec3 in_position;

uniform mat4 u_mvp;
uniform vec3 camera_position;
uniform float tile_size;

void main()
{
    vec4 tmp = u_mvp * vec4(in_position, 1.0);
    // tmp.xy += tile_size * floor(camera_position.xy / tile_size);
    gl_Position = tmp;
}
