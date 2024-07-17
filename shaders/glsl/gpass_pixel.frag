#version 450

#extension GL_GOOGLE_include_directive: require

#include "core.h"

layout (binding = TEXTURE_ARRAY_BINDING) uniform sampler2D textures[1024];

layout (location = 0) in vec3 normal;
layout (location = 1) in vec2 uv;
layout (location = 2) in flat uint draw_id;

layout (location = 0) out vec3 out_normal;
layout (location = 1) out vec4 out_albedoRGB_specularA;

void main()
{
    out_normal = normal;

    draw_info_t draw_info = draw_infos.draws[draw_id];

    vec4 color = vec4((draw_info.color >> 16 & 255)/255.0f, (draw_info.color >> 8 & 255)/255.0f, (draw_info.color & 255)/255.0f, 1.0);

    if(draw_info.texture_id != 0)
        color *= texture(textures[draw_info.texture_id], uv);

    //TODO: add specular mapping from texture
    out_albedoRGB_specularA = vec4(color.rgb, draw_info.specularity/2048.0);
}