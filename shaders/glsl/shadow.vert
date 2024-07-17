#version 450

#extension GL_GOOGLE_include_directive: require

#include "core.h"

#include "time.h"
#include "shader_ids.h"
#include "wave.h"
#include "camera.h"

SPEC_CONSTANT_BDA(0, camera_ubo_t, shadow_camera)
SPEC_CONSTANT_BDA(1, time_ubo_t, time_ubo)

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec3 v_normal;
layout (location = 2) in vec2 v_tex_coord;

void main()
{
    draw_info_t info = draw_infos.draws[gl_InstanceIndex];

    vec4 world_position = info.transform * vec4(v_position, 1.0);

    // we need to run the water shader in the shadow map as well to get accurate shadows.
    // this isnt ideal, but it works reasonably well for now
    switch(info.shader_id)
    {
        case WATER_SHADER:
            world_position.y = get_wave_data(world_position, time_ubo.time).y;
            break;
    }

    gl_Position = shadow_camera.projection * shadow_camera.view * world_position;
}