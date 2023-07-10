#version 330 core

layout (location = 0) in vec3 in_position;

uniform vec3 camera_position;
uniform float tile_size;
uniform mat4 u_mvp;

void main()
{
    vec3 pos = in_position;
    pos.xz += tile_size * round((camera_position.xz - pos.xz) / tile_size);
    gl_Position = vec4(pos, 1.0);
}
