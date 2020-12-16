#version 330 core

#myinclude_light

out vec4 o_frag_color;

in vec3 position;
in vec2 texcoord;

uniform sampler2D u_droplet_tex;

void main() {
    o_frag_color = sky_grey * 1.2;
    float alpha = 1 - texture(u_droplet_tex, texcoord).x;
    
    o_frag_color.a = alpha;
    // o_frag_color = texture(u_droplet_tex, texcoord);
    // o_frag_color = vec4(1, 1, position.z / 10,0);
}