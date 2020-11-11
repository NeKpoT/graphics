
#define DIR_LIGHT_SOURCES 1

uniform vec3 background_light;

uniform int dl_num;
uniform vec3 dl_dir[DIR_LIGHT_SOURCES];
uniform vec3 dl_light[DIR_LIGHT_SOURCES];
uniform sampler2D dl_depth[DIR_LIGHT_SOURCES];
uniform mat4 dl_vp[DIR_LIGHT_SOURCES];

#define PD_LIGHT_SOURCES 1

uniform int pd_num;
uniform vec3 pd_dir[PD_LIGHT_SOURCES];
uniform vec3 pd_pos[PD_LIGHT_SOURCES];
uniform vec3 pd_light[PD_LIGHT_SOURCES];
uniform float pd_angle[PD_LIGHT_SOURCES];

float get_shadow(vec3 obj_pos, int dl_i) {
    vec3 scoord = (dl_vp[dl_i] * vec4(obj_pos, 1)).xyz / (dl_vp[dl_i] * vec4(obj_pos, 1)).w;
    scoord = scoord / 2 + 0.5;
    float shadow_depth = texture(dl_depth[dl_i], scoord.xy).x;

    // return scoord.z - shadow_depth + 1e-3;
    if (scoord.z - 1e-3 >= shadow_depth)
        return 0;
    else
        return 1;
}

vec3 light_to_color(vec3 light) {
    return light;
}

vec3 get_light(vec3 obj_position, vec3 camera_position, vec3 normal, vec3 texture_albedo, float glossyness_a) {

    vec3 obj_eye = normalize(camera_position - obj_position);

    vec3 res_light = background_light;

    for (int dl_i = 0; dl_i < dl_num; dl_i++) {
        vec3 light_e = dl_light[dl_i] * max(0, dot(dl_dir[dl_i], normal));
        light_e = mix(
            light_e,
            light_e * max(0, dot(obj_eye, reflect(dl_dir[dl_i], normal))),
            glossyness_a
        );
        res_light += light_e * get_shadow(obj_position, dl_i);
    }

    for (int pd_i = 0; pd_i < pd_num; pd_i++) {
        vec3 source_obj = obj_position - pd_pos[pd_i];
        float pd_dist = length(source_obj);
        source_obj /= pd_dist;

        if (dot(source_obj, pd_dir[pd_i]) < pd_angle[pd_i])
            continue;

        vec3 light_e = pd_light[pd_i] * max(0, dot(-source_obj, normal)) / pd_dist / pd_dist;

        light_e = mix(
            light_e,
            light_e * max(0, dot(obj_eye, reflect(-source_obj, normal))),
            glossyness_a
        );
        res_light += light_e;
    }
    
    res_light *= texture_albedo;

    return light_to_color(res_light);
}

vec4 texture_cubemap(samplerCube cube, vec3 dir) {
    dir = normalize(dir);
    vec4 res = texture(cube, dir);

    const float FULL_COLOR = 0.995;
    const float NO_COLOR = 0.993;

    for (int i = 0; i < dl_num; i++) {
        float dot_v = dot(dl_dir[i], dir);
        if (dot_v > FULL_COLOR) {
            res = vec4(1, 1, 1, 1);
            res += vec4(dl_light[i], 0);
        } else if (dot_v > NO_COLOR) {
            res += vec4(dl_light[i], 0) * (dot_v - NO_COLOR) / (FULL_COLOR - NO_COLOR);
        }
    }

    // res = min(res, vec4(1, 1, 1, 1));

    return res;
}