#version 450

#extension GL_GOOGLE_include_directive: require

#include "core.h"

BUFFER_REFERENCE(readonly buffer texture_ids_t
{
    uint ssao;
})

SPEC_CONSTANT_BDA(0, texture_ids_t, texture_ids)

layout (binding = TEXTURE_ARRAY_BINDING) uniform sampler2D textures[1024];

layout (location = 0) in vec2 frag_uv;
layout (location = 0) out float out_occlusion_factor;

float offsets[4] = float[]( -1.5, -0.5, 0.5, 1.5);

void main()
{
    float result = 0;
    vec2 tex_size = 1.0/GET_TEXTURE_SIZE(texture_ids.ssao);

    for(int i = 0; i < 4; i++)
    {
        for(int j = 0; j < 4; j++)
        {
            vec2 offset = vec2(offsets[j], offsets[i])*tex_size;
            result += GET_TEXTURE_VAL(texture_ids.ssao, frag_uv + offset).r;
        }
    }
    out_occlusion_factor = result/16.0;
}