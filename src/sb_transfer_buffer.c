#include "sb_transfer_buffer.h"
#include "sb_vulkan_initializers.h"
#include "sb_common.h"
#include "sb_arena.h"
#include "sb_file.h"

#include <memory.h>
#include <stdio.h>

sb_transfer_buffer sb_create_transfer_buffer(VkDevice device,
	const sb_memory_types *memory_types, uint32_t transfer_queue_index, uint32_t graphics_queue_index)
{
	sb_transfer_buffer transfer_buffer = {0};

	transfer_buffer.graphics_queue = sb_get_queue(device, graphics_queue_index);
	transfer_buffer.graphics_queue_index = graphics_queue_index;
	transfer_buffer.graphics_command_pool = sb_create_command_pool(device, graphics_queue_index);
	sb_create_command_buffers(device, transfer_buffer.graphics_command_pool, &transfer_buffer.graphics_command_buffer, 1);

	if(graphics_queue_index != transfer_queue_index)
	{
		transfer_buffer.transfer_queue = sb_get_queue(device, transfer_queue_index);
		transfer_buffer.transfer_queue_index = transfer_queue_index;
		transfer_buffer.transfer_command_pool = sb_create_command_pool(device, transfer_queue_index);
		transfer_buffer.transfer_queue_finished = sb_create_semaphore(device);
		sb_create_command_buffers(device, transfer_buffer.transfer_command_pool, &transfer_buffer.transfer_command_buffer, 1);
	}

	transfer_buffer.transfer_fully_finished = sb_create_fence(device, false);

	sb_memory_info staging_memory_info = {0};
	staging_memory_info.capacity = MB(256);
	staging_memory_info.memory_usage = SB_MEMORY_USAGE_CPU;
	staging_memory_info.buffer_usage_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	staging_memory_info.memory_types = memory_types;
	sb_allocate_device_arena(device, &staging_memory_info, &transfer_buffer.staging_memory);

	return transfer_buffer;
}

void sb_queue_mesh_transfer(sb_transfer_buffer *transfer_buffer, const char *file_path)
{
    sb_mesh_transfer *mesh_transfer = &transfer_buffer->mesh_transfers[transfer_buffer->mesh_transfer_count++];

    mesh_transfer->mesh_file = sb_fopen(file_path, "rb");

    fread(mesh_transfer, sizeof(uint32_t), 2, mesh_transfer->mesh_file);

	transfer_buffer->indices_to_transfer += mesh_transfer->index_count;
	transfer_buffer->vertices_to_transfer += mesh_transfer->vertex_count;
}

sb_texture_transfer *sb_queue_texture_transfer(sb_transfer_buffer *transfer_buffer)
{
	return &transfer_buffer->texture_transfers[transfer_buffer->texture_transfer_count++];
}

VkBufferMemoryBarrier2 get_transfer_queue_release_barrier(sb_buffer *buffer, uint32_t transfer_index, uint32_t graphics_index)
{
	VkBufferMemoryBarrier2 buffer_barrier = sb_get_buffer_barrier(buffer);
	buffer_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	buffer_barrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	buffer_barrier.dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	buffer_barrier.srcQueueFamilyIndex = transfer_index;
	buffer_barrier.dstQueueFamilyIndex = graphics_index;	

	return buffer_barrier;
}

VkBufferMemoryBarrier2 get_graphics_queue_acquire_barrier(sb_buffer *buffer, uint32_t transfer_index, uint32_t graphics_index)
{
	VkBufferMemoryBarrier2KHR buffer_barrier = sb_get_buffer_barrier(buffer);
    buffer_barrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT_KHR;
    buffer_barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT_KHR;
    buffer_barrier.srcQueueFamilyIndex = transfer_index;
    buffer_barrier.dstQueueFamilyIndex = graphics_index;
	return buffer_barrier;
}

void queue_image_transition_barriers(VkCommandBuffer command_buffer, sb_texture textures[SB_MAX_TEXTURES], sb_texture_transfer *transfers, uint32_t transfer_count, const VkImageMemoryBarrier2 *template_barrier)
{
	sb_arena_temp scratch = sb_get_scratch();
	VkImageMemoryBarrier2 *barriers = sb_arena_push(scratch.arena, VkImageMemoryBarrier2, transfer_count);

	for(uint16_t i = 0; i < transfer_count; i++)
	{
		sb_texture_transfer *transfer = &transfers[i];
		sb_texture *texture = &textures[transfer->texture_id];

		VkImageMemoryBarrier2 *barrier = &barriers[i];
		memcpy(barrier, template_barrier, sizeof(VkImageMemoryBarrier2));
		barrier->image = texture->image;
	}

	sb_image_barriers(command_buffer, barriers, transfer_count);
	sb_release_scratch(&scratch);
}

void sb_transfer_assets(VkDevice device, sb_transfer_buffer *transfer_buffer, sb_mesh_memory *meshes, sb_texture textures[SB_MAX_TEXTURES])
{
	bool has_mesh_transfers = transfer_buffer->mesh_transfer_count > 0;
	bool has_texture_transfers = transfer_buffer->texture_transfer_count > 0;
	if(!has_mesh_transfers && !has_texture_transfers) return;

	VkCommandBuffer main_command_buffer = VK_NULL_HANDLE;
	if(transfer_buffer->transfer_command_buffer)
	{
		main_command_buffer = transfer_buffer->transfer_command_buffer;
		sb_begin_command_buffer(transfer_buffer->graphics_command_buffer, true);
	}
	else main_command_buffer = transfer_buffer->graphics_command_buffer;

	sb_begin_command_buffer(main_command_buffer, true);

	if(has_mesh_transfers)
	{
		VkDeviceSize vertex_transfer_size = transfer_buffer->vertices_to_transfer * sizeof(sb_vertex);
		VkDeviceSize index_transfer_size = transfer_buffer->indices_to_transfer * sizeof(uint32_t);
		VkDeviceSize handle_transfer_size = transfer_buffer->mesh_transfer_count * sizeof(sb_mesh_handle);

		VkDeviceSize vertex_scratch_offset = sb_offset_device_arena_aligned(&transfer_buffer->staging_memory, vertex_transfer_size, _Alignof(sb_vertex));
		VkDeviceSize index_scratch_offset = sb_offset_device_arena_aligned(&transfer_buffer->staging_memory, index_transfer_size, _Alignof(uint32_t));
		VkDeviceSize handle_scratch_offset = sb_offset_device_arena_aligned(&transfer_buffer->staging_memory, handle_transfer_size, _Alignof(sb_mesh_handle));

		sb_vertex *vertex_scratch = sb_get_ptr(&transfer_buffer->staging_memory, vertex_scratch_offset);
		uint32_t *index_scratch = sb_get_ptr(&transfer_buffer->staging_memory, index_scratch_offset);
		sb_mesh_handle *handle_scratch = sb_get_ptr(&transfer_buffer->staging_memory, handle_scratch_offset);

		uint32_t starting_vertex_offset = meshes->vertex_count * sizeof(sb_vertex);
		uint32_t starting_index_offset = meshes->index_count * sizeof(uint32_t);
		uint32_t starting_handle_offset = meshes->handle_count * sizeof(sb_mesh_handle);

		uint32_t vertices_read = 0;
		uint32_t indices_read = 0;

		for(int meshes_read = 0; meshes_read < transfer_buffer->mesh_transfer_count; meshes_read++)
		{
			sb_mesh_transfer *mesh_transfer = &transfer_buffer->mesh_transfers[meshes_read];

			fread(&vertex_scratch[vertices_read], sizeof(sb_vertex), mesh_transfer->vertex_count, mesh_transfer->mesh_file);
			fread(&index_scratch[indices_read], sizeof(uint32_t), mesh_transfer->index_count, mesh_transfer->mesh_file);

			sb_mesh_handle *handle  = &handle_scratch[meshes_read];
			handle->vertex_offset = meshes->vertex_count + vertices_read;
			handle->vertex_count = mesh_transfer->vertex_count;
			handle->first_index = meshes->index_count + indices_read;
			handle->index_count = mesh_transfer->index_count;

			vertices_read += mesh_transfer->vertex_count;
			indices_read += mesh_transfer->index_count;

			fclose(mesh_transfer->mesh_file);
		}

		meshes->vertex_count += vertices_read;
		meshes->index_count  += indices_read;
		meshes->handle_count += transfer_buffer->mesh_transfer_count;

		sb_buffer_copy_info vertex_copy = {0};
		vertex_copy.src_buffer = &transfer_buffer->staging_memory;
		vertex_copy.src_offset = vertex_scratch_offset;
		vertex_copy.dst_buffer = &meshes->vertex_buffer;
		vertex_copy.dst_offset = starting_vertex_offset;
		vertex_copy.size = vertex_transfer_size;
		sb_buffer_copy(main_command_buffer, &vertex_copy);

		sb_buffer_copy_info index_copy = {0};
		index_copy.src_buffer = &transfer_buffer->staging_memory;
		index_copy.src_offset = index_scratch_offset;
		index_copy.dst_buffer = &meshes->index_buffer;
		index_copy.dst_offset = starting_index_offset;
		index_copy.size = index_transfer_size;
		sb_buffer_copy(main_command_buffer, &index_copy);

		sb_buffer_copy_info handle_copy = {0};
		handle_copy.src_buffer = &transfer_buffer->staging_memory;
		handle_copy.src_offset = handle_scratch_offset;
		handle_copy.dst_buffer = &meshes->handle_buffer;
		handle_copy.dst_offset = starting_handle_offset;
		handle_copy.size = handle_transfer_size;
		sb_buffer_copy(main_command_buffer, &handle_copy);

		if(transfer_buffer->transfer_queue) // separate transfer queue
		{
			VkBufferMemoryBarrier2 transfer_release[] = {
				get_transfer_queue_release_barrier(&meshes->vertex_buffer, transfer_buffer->transfer_queue_index, transfer_buffer->graphics_queue_index),
				get_transfer_queue_release_barrier(&meshes->index_buffer, transfer_buffer->transfer_queue_index, transfer_buffer->graphics_queue_index),
				get_transfer_queue_release_barrier(&meshes->handle_buffer, transfer_buffer->transfer_queue_index, transfer_buffer->graphics_queue_index),
			};

			sb_buffer_barriers(transfer_buffer->transfer_command_buffer, transfer_release, COUNTOF(transfer_release));

			VkBufferMemoryBarrier2 graphics_acquire[] = {
				get_graphics_queue_acquire_barrier(&meshes->vertex_buffer, transfer_buffer->transfer_queue_index, transfer_buffer->graphics_queue_index),
				get_graphics_queue_acquire_barrier(&meshes->index_buffer, transfer_buffer->transfer_queue_index, transfer_buffer->graphics_queue_index),
				get_graphics_queue_acquire_barrier(&meshes->handle_buffer, transfer_buffer->transfer_queue_index, transfer_buffer->graphics_queue_index),
			};

			sb_buffer_barriers(transfer_buffer->graphics_command_buffer, graphics_acquire, COUNTOF(graphics_acquire));
		}
	}

	if(has_texture_transfers)
	{
		VkImageMemoryBarrier2 transfer_barrier = sb_get_image_layout_transition_barrier(VK_IMAGE_ASPECT_COLOR_BIT);
		transfer_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		transfer_barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		transfer_barrier.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;

		queue_image_transition_barriers(main_command_buffer,
			textures,
			transfer_buffer->texture_transfers,
			transfer_buffer->texture_transfer_count,
			&transfer_barrier
		);

		for(uint16_t i = 0; i < transfer_buffer->texture_transfer_count; i++)
		{
			sb_texture_transfer *transfer = &transfer_buffer->texture_transfers[i];
			sb_texture *texture = &textures[transfer->texture_id];

		   //TODO: remove stb_image as a dependency so i can just directly read into the staging memory. this is super annoying
			VkDeviceSize pixel_memory_size = texture->extent.width * texture->extent.height * 4;
			VkDeviceSize pixel_scratch_offset = sb_offset_device_arena(&transfer_buffer->staging_memory, char, pixel_memory_size);
			void *pixel_scratch = sb_get_ptr(&transfer_buffer->staging_memory, pixel_scratch_offset);
			memcpy(pixel_scratch, transfer->pixels, pixel_memory_size); 
			free(transfer->pixels);

			VkBufferImageCopy2 buffer_image_copy = {0};
			buffer_image_copy.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
			buffer_image_copy.bufferOffset = pixel_scratch_offset;
			buffer_image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			buffer_image_copy.imageSubresource.layerCount = 1;
			buffer_image_copy.imageExtent = (VkExtent3D) {texture->extent.width, texture->extent.height, 1};


			VkCopyBufferToImageInfo2 buffer_image_copy_info = {0};
			buffer_image_copy_info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
			buffer_image_copy_info.srcBuffer = transfer_buffer->staging_memory.vk_buffer;
			buffer_image_copy_info.dstImage = texture->image;
			buffer_image_copy_info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			buffer_image_copy_info.regionCount = 1;
			buffer_image_copy_info.pRegions = &buffer_image_copy;

			vkCmdCopyBufferToImage2(main_command_buffer, &buffer_image_copy_info);
		}

		VkImageMemoryBarrier2 shader_read_only_barrier = sb_get_image_layout_transition_barrier(VK_IMAGE_ASPECT_COLOR_BIT);
		shader_read_only_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		shader_read_only_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		shader_read_only_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		shader_read_only_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		shader_read_only_barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
		shader_read_only_barrier.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
		shader_read_only_barrier.srcQueueFamilyIndex = transfer_buffer->transfer_queue_index;
		shader_read_only_barrier.dstQueueFamilyIndex = transfer_buffer->graphics_queue_index;

		queue_image_transition_barriers(main_command_buffer,
			textures,
			transfer_buffer->texture_transfers,
			transfer_buffer->texture_transfer_count,
			&shader_read_only_barrier
		);

		if(transfer_buffer->transfer_queue)
		{
			VkImageMemoryBarrier2 ownership_transfer = sb_get_image_layout_transition_barrier(VK_IMAGE_ASPECT_COLOR_BIT);
			ownership_transfer.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			ownership_transfer.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			ownership_transfer.srcAccessMask = 0;
			ownership_transfer.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			ownership_transfer.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			ownership_transfer.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			ownership_transfer.srcQueueFamilyIndex = transfer_buffer->transfer_queue_index;
			ownership_transfer.dstQueueFamilyIndex = transfer_buffer->graphics_queue_index;

			queue_image_transition_barriers(main_command_buffer,
				textures,
				transfer_buffer->texture_transfers,
				transfer_buffer->texture_transfer_count,
				&ownership_transfer
			);
		}
	}

	sb_end_command_buffer(main_command_buffer);

	if(transfer_buffer->transfer_queue)
	{
		sb_end_command_buffer(transfer_buffer->graphics_command_buffer);

		sb_queue_submit_info submit_info = {0};
		submit_info.command_buffer = transfer_buffer->transfer_command_buffer;
		submit_info.signal_semaphore = transfer_buffer->transfer_queue_finished;
		sb_queue_submit(transfer_buffer->transfer_queue, &submit_info);
	}

	sb_queue_submit_info submit_info = {0};
	submit_info.command_buffer = main_command_buffer;
	submit_info.wait_semaphore = transfer_buffer->transfer_queue_finished;
	submit_info.wait_stage_mask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
	submit_info.fence = transfer_buffer->transfer_fully_finished;
	sb_queue_submit(transfer_buffer->graphics_queue, &submit_info);

	sb_wait_for_fence(device, transfer_buffer->transfer_fully_finished);
	sb_reset_fence(device, transfer_buffer->transfer_fully_finished);

	transfer_buffer->indices_to_transfer = 0;
	transfer_buffer->vertices_to_transfer = 0;
	transfer_buffer->mesh_transfer_count = 0;
	transfer_buffer->texture_transfer_count = 0;

	sb_reset_device_arena(&transfer_buffer->staging_memory);

	sb_reset_command_pool(device, transfer_buffer->graphics_command_pool);
	sb_create_command_buffers(device, transfer_buffer->graphics_command_pool, &transfer_buffer->graphics_command_buffer, 1);

	if(transfer_buffer->transfer_queue)
	{
		sb_reset_command_pool(device, transfer_buffer->transfer_command_pool);
		sb_create_command_buffers(device, transfer_buffer->transfer_command_pool, &transfer_buffer->transfer_command_buffer, 1);
	}
}