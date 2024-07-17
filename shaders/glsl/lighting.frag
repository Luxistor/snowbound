#version 450

#extension GL_GOOGLE_include_directive: require

#include "core.h"
#include "camera.h"

BUFFER_REFERENCE(readonly buffer lighting_ubo_t
{
  uint ssao_id;
  uint depth_buffer_id;
  uint normal_id;
  uint albedoRGB_specularityA_id;

  uint shadow_map_id;
  uint pad0[3];

  vec4 sun_direction;

  mat4 scene_view_to_shadow_light_space_matrix;
})

SPEC_CONSTANT_BDA(0, lighting_ubo_t, lighting_ubo)
SPEC_CONSTANT_BDA(1, camera_ubo_t, camera_ubo)

layout (binding = TEXTURE_ARRAY_BINDING) uniform sampler2D textures[1024];

layout (location = 0) in vec2 screen_coords;
layout (location = 0) out vec4 out_color;

float shadow_calc(vec3 view_pos)
{
  vec4 shadow_pos = lighting_ubo.scene_view_to_shadow_light_space_matrix * vec4(view_pos, 1.0);
  
  // PCF
  float shadow = 0;
  vec2 texel_size = 1.0 / GET_TEXTURE_SIZE(lighting_ubo.shadow_map_id);
  for(int x = -1; x <= 1; ++x)
  {
    for(int y = -1; y <= 1; ++y)
    {
        float depth = GET_TEXTURE_VAL(lighting_ubo.shadow_map_id, shadow_pos.xy + vec2(x,y)*texel_size).r;
        shadow += depth < shadow_pos.z ? 0.0 : 1.0;
    }
  }

	return shadow / 9.0;
}

const float sun_intensity = 0.5;
const float ambient_intensity = 0.5;
const vec4 specular_color = vec4(1,0.984,0,1.0);

void main()
{
  float occlusion_factor = GET_TEXTURE_VAL(lighting_ubo.ssao_id, screen_coords).r;

  float depth = GET_TEXTURE_VAL(lighting_ubo.depth_buffer_id, screen_coords).r;
  vec3 view_pos = get_view_from_ndc(camera_ubo.projection, vec3(screen_coords, depth)); // vertex position in view space

  vec3 normal = GET_TEXTURE_VAL(lighting_ubo.normal_id, screen_coords).rgb; // also in view space
  normal = normalize(normal);

  vec4 albedo = vec4(GET_TEXTURE_VAL(lighting_ubo.albedoRGB_specularityA_id, screen_coords).rgb,1.0);
  float specularity = GET_TEXTURE_VAL(lighting_ubo.albedoRGB_specularityA_id, screen_coords).a;

  // lamberts cosine law
	// the intensity of the light is proportional to the angle between the normal and the direction of the light
  float diffuse_intensity = max(dot(normal, lighting_ubo.sun_direction.xyz), 0.0);
  vec4 diffuse_temp = diffuse_intensity*albedo;
  diffuse_temp = sun_intensity*clamp(diffuse_temp,vec4(0), albedo);

	// specularity
  vec4 specular_temp = vec4(0);
  if(specularity > 0)
  {
    // since we're in view space
    vec3 view_direction = normalize(-view_pos);
    // optimization over using the reflection vector
    vec3 halfway = normalize(lighting_ubo.sun_direction.xyz + view_direction);
    float specular_intensity = max(dot(halfway, normal),0.0);

    specular_temp = specular_color*albedo*pow(specular_intensity, specularity*2048.0);
  }

  out_color = ambient_intensity*albedo*occlusion_factor + (diffuse_temp+specular_temp)*shadow_calc(view_pos);
}