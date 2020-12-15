#version 330 core

out vec4 o_frag_color;

in vec3 position;
in vec2 texcoord;

void main() {
    o_frag_color = vec4(1, 0, 0,0);
    // o_frag_color = vec4(1, 1, position.z / 10,0);
}