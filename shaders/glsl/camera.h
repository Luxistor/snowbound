#include "core.h"

BUFFER_REFERENCE(readonly buffer camera_ubo_t
{
	mat4 projection;
	mat4 view;
})

float get_view_z_from_ndc(mat4 projection, float ndc)
{
    return projection[3][2] /(ndc - projection[2][2]);
}

vec3 get_view_from_ndc(mat4 projection, vec3 ndc)
{
    float view_z = get_view_z_from_ndc(projection, ndc.z);
    return vec3(
        (ndc.x*2.0-1.0) * (view_z/projection[0][0]),
        (ndc.y*2.0-1.0) * (view_z/projection[1][1]),
        view_z
    );
}