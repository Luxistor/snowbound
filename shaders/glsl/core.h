#ifndef CORE_H
#define CORE_H

#define _DEBUG

#ifdef _DEBUG

#extension GL_EXT_debug_printf : enable
#define PRINT_VEC3(vec)  debugPrintfEXT("x:%f y:%f z:%f\n", vec.x, vec.y, vec.z);

#endif

#extension GL_EXT_buffer_reference: require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable

#define DRAW_INFO_BUFFER_BINDING (0U)
#define TEXTURE_ARRAY_BINDING 1U
#define DESCRIPTOR_BINDING_COUNT (TEXTURE_ARRAY_BINDING+1)U

struct draw_info_t
{
	mat4 transform;

	uint mesh_id;
	uint texture_id;
    float specularity;
    uint shader_id;

    uint color;
    uint pad[3];
};

layout (binding = DRAW_INFO_BUFFER_BINDING) readonly buffer DrawInfos
{
    uint count;
    draw_info_t[] draws;
} draw_infos;

#define _PTR_NAME(type,id) type ## _ptr ## id
#define SPEC_CONSTANT_BDA(id, type, name)\
	layout(constant_id = id) const uint64_t _PTR_NAME(type,id) = 0;\
	type name = type(_PTR_NAME(type,id));
#define BUFFER_REFERENCE(type) layout(std430, buffer_reference, buffer_reference_align = 16) type;

#define GET_TEXTURE_VAL(id, offset) texture(textures[id], offset)
#define GET_TEXTURE_SIZE(id) textureSize(textures[id], 0)

#endif