#version 330 core

#myinclude_light

out vec4 o_frag_color;

in vec3 position;
in vec2 texcoord;

uniform sampler2D u_droplet_tex;

uniform sampler2D u_shadow_tex;
uniform mat4 u_shadow_view;

int get_shadow(vec3 obj_pos) {
    vec4 scoord = u_shadow_view * vec4(obj_pos, 1);
    scoord /= scoord.w;
    scoord = scoord / 2 + 0.5;
    float shadow_depth = texture(u_shadow_tex, scoord.xy).x;

    if (scoord.z - 1e-3 >= shadow_depth)
        return 0;
    else
        return 1;
}

void main() {
    int shadow = get_shadow(position);

    o_frag_color = sky_grey * (0.7 + 0.5 * get_shadow(position));

    float alpha = 1 - texture(u_droplet_tex, texcoord).x;
    
    o_frag_color.a = alpha;
    // o_frag_color = vec4(texture(u_droplet_tex, texcoord).x, 0, 0, 1);
    // o_frag_color = vec4(1, 1, position.z / 10,0);
}