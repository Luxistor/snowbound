#version 450

#extension GL_GOOGLE_include_directive: require

#include "core.h"

#include "time.h"
#include "camera.h"
#include "wave.h"
#include "shader_ids.h"

SPEC_CONSTANT_BDA(0, time_ubo_t, time_ubo)
SPEC_CONSTANT_BDA(1, camera_ubo_t, scene_camera)

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec3 v_normal;
layout (location = 2) in vec2 v_uv;

layout (location = 0) out vec3 out_normal;
layout (location = 1) out vec2 out_uv;
layout (location = 2) out flat uint draw_id;

const int repeat_count = 3;

void main()
{
    draw_info_t info = draw_infos.draws[gl_InstanceIndex];

    vec4 world_position = info.transform * vec4(v_position, 1.0);
    vec3 normal = v_normal;
    vec2 uv = v_uv;
    // branching on constants like this is free on modern gpus, the main concern is register pressure
    switch(info.shader_id)
    {
        case WATER_SHADER:
			wave_result_t result = get_wave_data(world_position, time_ubo.time);

            vec3 binormal  = vec3(1, result.dydx, 0);
            vec3 tangent = vec3(0, result.dydz, 1);

            normal = normalize(cross(tangent,binormal));

            world_position.y = result.y;
            break;
        case ICE_SHADER:
            uv = vec2( world_position.x / repeat_count, world_position.z / repeat_count);
            break;
    }   

    vec4 view_pos = scene_camera.view * world_position;

    gl_Position = scene_camera.projection * view_pos;

    out_uv = uv;
    out_normal = transpose(inverse(mat3(scene_camera.view * info.transform))) * normal;
    draw_id = gl_InstanceIndex;
} 