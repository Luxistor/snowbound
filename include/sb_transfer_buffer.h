#ifndef SB_TRANSFER_BUFFER_H
#define SB_TRANSFER_BUFFER_H

#include <stdio.h>

#include "sb_mesh.h"
#include "sb_texture.h"

typedef struct
{
	uint32_t vertex_count;
	uint32_t index_count;
	FILE *mesh_file;
} sb_mesh_transfer;

typedef struct
{
	sb_texture_id texture_id;
	char *pixels;
} sb_texture_transfer;

typedef struct
{
	VkQueue graphics_queue; // if there is not, we use the graphics queue, but if there is, we still use it for queue family ownership transfer
	uint32_t graphics_queue_index;
    VkCommandPool graphics_command_pool;
    VkCommandBuffer graphics_command_buffer;

	VkQueue transfer_queue; // if there is a dedicated transfer queue, we use this to schedule transfer commands
	uint32_t transfer_queue_index;
	VkCommandPool transfer_command_pool;
	VkSemaphore transfer_queue_finished; // the transfer queue is finished and now we hand off to the graphics queue to finish ownership transfer
    VkCommandBuffer transfer_command_buffer;

	VkFence transfer_fully_finished; // transfer is fully complete

    sb_device_arena staging_memory;

	uint32_t vertices_to_transfer;
	uint32_t indices_to_transfer;

    sb_mesh_transfer mesh_transfers[SB_MAX_MESHES];
    uint32_t mesh_transfer_count;

    sb_texture_transfer texture_transfers[SB_MAX_TEXTURES];
    uint32_t texture_transfer_count;
} sb_transfer_buffer;

sb_transfer_buffer sb_create_transfer_buffer(VkDevice device,
	const sb_memory_types *memory_types, uint32_t transfer_queue_index, uint32_t graphics_queue_index);

static VkBufferMemoryBarrier2 get_transfer_queue_release_barrier(sb_buffer *buffer, uint32_t transfer_index, uint32_t graphics_index);
static VkBufferMemoryBarrier2 get_graphics_queue_acquire_barrier(sb_buffer *buffer, uint32_t transfer_index, uint32_t graphics_index);

void queue_image_transition_barriers(VkCommandBuffer command_buffer, sb_texture textures[SB_MAX_TEXTURES], sb_texture_transfer *transfers, uint32_t transfer_count, const VkImageMemoryBarrier2 *template_barrier);

void sb_queue_mesh_transfer(sb_transfer_buffer *transfer_buffer, const char *file_path);
sb_texture_transfer *sb_queue_texture_transfer(sb_transfer_buffer *transfer_buffer);
void sb_transfer_assets(VkDevice device, sb_transfer_buffer *transfer_buffer, sb_mesh_memory *meshes, sb_texture textures[SB_MAX_TEXTURES]);

#endif
