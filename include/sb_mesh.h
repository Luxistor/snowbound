#ifndef SB_MESH_H
#define SB_MESH_H

#include "sb_math.h"
#include "sb_vulkan_memory.h"

#define SB_MAX_MESHES 1024U

typedef uint32_t sb_mesh_id;

typedef struct
{
	sb_vec3 position;
	sb_vec3 normal;
	sb_vec2 uv;
} sb_vertex;

typedef struct
{
	int vertex_offset;
	uint32_t vertex_count;

	uint32_t first_index;
	uint32_t index_count;
} sb_mesh_handle;

typedef struct
{
	uint32_t vertex_count;
	sb_buffer vertex_buffer;

	uint32_t index_count;
	sb_buffer index_buffer;

	uint32_t handle_count;
	sb_buffer handle_buffer;

	uint32_t mesh_count;
} sb_mesh_memory;

sb_mesh_memory sb_alloc_mesh_memory(VkDevice device, const sb_memory_types *memory_types);
void sb_bind_mesh_buffers(VkCommandBuffer command_buffer, sb_mesh_memory *meshes);


#endif
