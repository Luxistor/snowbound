#version 450

#extension GL_GOOGLE_include_directive: require

#include "core.h"

#include "camera.h"

#define SAMPLE_COUNT 128
#define SAMPLE_RADIUS 1.5f
#define OCCLUSION_STRENGTH 2.0
#define BIAS 0.025f

BUFFER_REFERENCE(readonly buffer ssao_ubo_t
{
    uint depth_buffer_id;
    uint pad[3];
    vec4 offsets[SAMPLE_COUNT];
})

SPEC_CONSTANT_BDA(0, ssao_ubo_t, ssao_ubo)
SPEC_CONSTANT_BDA(1, camera_ubo_t, camera_ubo)

layout (binding = TEXTURE_ARRAY_BINDING) uniform sampler2D textures[1024];

layout (location = 0) in vec2 frag_uv;
layout (location = 0) out float out_occlusion_factor;

void main()
{   
    float occlusion_factor = SAMPLE_COUNT;

    float depth = GET_TEXTURE_VAL(ssao_ubo.depth_buffer_id, frag_uv).r;
    vec3 vertex_position = get_view_from_ndc(camera_ubo.projection, vec3(frag_uv, depth)); // vertex position in view space

    // samples are in a unit sphere around the vertex position point
    for(int i = 0; i < SAMPLE_COUNT; i++)
    {
        vec4 sample_position = vec4(vertex_position,1.0) + ssao_ubo.offsets[i]; // we now offset to get the sample point in view space
        sample_position = camera_ubo.projection * sample_position; // project the offset onto the near plane
        sample_position.xy /=  sample_position.w; // perspective divide
        sample_position.xy = sample_position.xy*0.5 + 0.5; // map xy into screen space coordinates so we can read from the vertex texture

        //  now we can read the actual vertex stored along the camera ray
        float sample_depth = GET_TEXTURE_VAL(ssao_ubo.depth_buffer_id, sample_position.xy).r;
        sample_depth = get_view_z_from_ndc(camera_ubo.projection, sample_depth);

        float dist = abs(vertex_position.z - sample_depth);
        // gives less weight to vertices that are further away
        float range_weight = smoothstep(0.0, 1.0, SAMPLE_RADIUS/dist);

        // if the samples geometry is in front of the current fragment
        if(sample_depth >= (vertex_position.z + BIAS))
        {   
            // the current vertice is being occluded by some geometry, so we should decrease the occlusion factor
            // this means that the ambient light factor will be reduced in the lighting pass
            occlusion_factor -= range_weight;
        }
    }

    // map to [0,1]
    occlusion_factor /= SAMPLE_COUNT;
    out_occlusion_factor = pow(occlusion_factor, OCCLUSION_STRENGTH);
}