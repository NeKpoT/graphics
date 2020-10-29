#version 330 core

out vec4 o_frag_color;

struct vx_output_t
{
    vec3 world_position;
};

in vx_output_t v_out;

uniform samplerCube u_cube;

void main()
{
    vec3 texture = texture(u_cube, v_out.world_position).rgb; 
    o_frag_color = vec4(texture,1.0);
}
