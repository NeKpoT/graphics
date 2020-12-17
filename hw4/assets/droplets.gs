#version 330 core

layout (points) in;
layout (triangle_strip, max_vertices = 6) out;

out vec3 position;
out vec2 texcoord;

uniform vec3 camera_position;
uniform vec3 camera_forward;
uniform mat4 u_mvp;

uniform float width = 0.005;
uniform float height = 0.2;

uniform mat4 u_height_view;
uniform sampler2D u_height_tex;

float get_shadow(vec3 obj_pos) {
    vec4 scoord = u_height_view * vec4(obj_pos, 1);
    scoord /= scoord.w;
    scoord = scoord / 2 + 0.5;
    float shadow_depth = texture(u_height_tex, scoord.xy).x;

    if (scoord.z - 1e-3 >= shadow_depth)
        return 0;
    else
        return 1;
}

void main()
{
    //position = vec3(0.0, 0.0, 0.0);
    //texcoord = position.xy;
    //gl_Position = vec4(position, 1.0);
    //EmitVertex();
    //position = vec3(1.0, 0.0, 0.0);
    //texcoord = position.xy;
    //gl_Position = vec4(position, 1.0);
    //EmitVertex();
    //position = vec3(1.0, 1.0, 0.0);
    //texcoord = position.xy;
    //gl_Position = vec4(position, 1.0);
    //EmitVertex();
    //EndPrimitive();

    vec3 point_pos = gl_in[0].gl_Position.xyz;

    if (get_shadow(point_pos) == 0) {
        EndPrimitive();
        return;
    }

    vec3 camera_up = vec3(0, 1, 0);
    vec3 camera_fw_plane = camera_forward - camera_up * dot(camera_forward, camera_up);
    vec3 camera_left = cross(camera_forward, camera_up);

    vec3 up = camera_up * height;
    vec3 left = camera_left * (width / 2);

    float left_mult[6] = float[6]( +1, -1, -1, -1, +1, +1 );
    float up_mult[6] =   float[6]( -1, -1, +1, +1, +1, -1 );

    for (int tr = 0; tr < 2; tr++) {
        for (int v = 0; v < 3; v++) {
            int i = tr * 3 + v;
            position = point_pos + left * left_mult[i] + up * (up_mult[i] + 1) / 2;
            gl_Position = u_mvp * vec4(position, 1.0);
            texcoord = (vec2(1 - left_mult[i], 1 + up_mult[i])) / 2;
            EmitVertex();
        }
        EndPrimitive();
    }
}
