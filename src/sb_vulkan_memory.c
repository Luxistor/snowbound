#include "sb_vulkan_memory.h"
#include "sb_math.h"
#include "sb_common.h"

bool is_host_visible(sb_memory_usage memory_usage)
{
	return memory_usage == SB_MEMORY_USAGE_CPU || memory_usage == SB_MEMORY_USAGE_CPU_TO_GPU;
}

uint32_t sb_get_memory_index(const sb_memory_types *memory_types,
	uint32_t memory_type_bits_requirement, sb_memory_usage memory_usage)
{
	VkMemoryPropertyFlags required_properties = memory_usage;
	for (uint32_t memory_index = 0; memory_index < memory_types->memory_type_count; memory_index++)
	{
		const uint32_t memory_type_bits = (1 << memory_index);

		const bool is_required_memory_type = memory_type_bits_requirement & memory_type_bits;
		const bool has_required_properties = (memory_types->memory_types[memory_index].propertyFlags & required_properties) == required_properties;

		if (is_required_memory_type && has_required_properties)
		{
			return memory_index;
		}
	}

	if(memory_usage == SB_MEMORY_USAGE_CPU_TO_GPU || memory_usage == SB_MEMORY_USAGE_GPU)
	{
		return sb_get_memory_index(memory_types, memory_type_bits_requirement, SB_MEMORY_USAGE_CPU);
	}

	SB_PANIC("Cant find requested memory index!");
}

sb_memory_types sb_get_memory_types(VkPhysicalDevice physicalDevice)
{
	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memory_properties);

	sb_memory_types mem_types;
	mem_types.memory_type_count = memory_properties.memoryTypeCount;
	memcpy(mem_types.memory_types, memory_properties.memoryTypes, sizeof(VkMemoryType) * memory_properties.memoryTypeCount);
	return mem_types;
}

VkBuffer create_vk_buffer(VkDevice device, VkDeviceSize capacity, VkBufferUsageFlags buffer_usage)
{
	VkBufferCreateInfo buffer_create_info = {0};
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buffer_create_info.size = capacity;
	buffer_create_info.usage = buffer_usage;

	VkBuffer buffer;
	VK_CHECK(vkCreateBuffer(device, &buffer_create_info, NULL, &buffer));
	return buffer;
}

VkDeviceMemory allocate_and_bind_memory(VkDevice device, VkBuffer buffer, sb_memory_usage memory_usage, const sb_memory_types *memory_types)
{
	VkMemoryRequirements buffer_requirements;
	vkGetBufferMemoryRequirements(device, buffer, &buffer_requirements);

	VkMemoryAllocateFlagsInfo flags_info = {0};
	flags_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
	flags_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

	VkMemoryAllocateInfo memory_allocate_info = {0};
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize = buffer_requirements.size;
	memory_allocate_info.pNext = &flags_info;
	memory_allocate_info.memoryTypeIndex = sb_get_memory_index(memory_types,buffer_requirements.memoryTypeBits, memory_usage);

	VkDeviceMemory memory;
	VK_CHECK(vkAllocateMemory(device, &memory_allocate_info, NULL, &memory));
	VK_CHECK(vkBindBufferMemory(device, buffer, memory, 0));
	return memory;
}

VkDeviceAddress get_address(VkDevice device, VkBuffer buffer)
{
	VkBufferDeviceAddressInfo device_address_info = {0};
	device_address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	device_address_info.buffer = buffer;
	return vkGetBufferDeviceAddress(device, &device_address_info);
}

void *map_memory(VkDevice device, VkDeviceMemory memory)
{
	void *ptr;
	VK_CHECK(vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, &ptr));
	return ptr;
}

void sb_allocate_buffer(VkDevice device, const sb_memory_info *info, sb_buffer *out_buffer)
{
	out_buffer->vk_buffer = create_vk_buffer(device, info->capacity, info->buffer_usage_flags | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
	out_buffer->memory = allocate_and_bind_memory(device, out_buffer->vk_buffer, info->memory_usage, info->memory_types);
	out_buffer->address = get_address(device, out_buffer->vk_buffer);
	out_buffer->memory_ptr = is_host_visible(info->memory_usage) ? map_memory(device, out_buffer->memory) : NULL;
	out_buffer->capacity = info->capacity;
}

void sb_allocate_device_arena(VkDevice device, const sb_memory_info *info, sb_device_arena *out_arena)
{
	sb_allocate_buffer(device, info, (sb_buffer*) out_arena);
	out_arena->offset = 0;
}

VkDeviceSize sb_offset_device_arena_aligned(sb_device_arena *arena, VkDeviceSize size, VkDeviceSize align)
{
	assert(sb_is_power_of_two(align));

	uintptr_t aligned_offset = sb_align_forward_power_of_two(arena->offset, align);
	size_t new_size = aligned_offset + size;

	assert(new_size < arena->capacity);
	arena->offset = new_size;

	return aligned_offset;
}

void *sb_get_ptr(sb_device_arena *arena, VkDeviceSize offset)
{
	return (char*)arena->memory_ptr + offset;
}
VkDeviceAddress sb_get_address(sb_device_arena *arena, VkDeviceSize offset)
{
	return arena->address + offset;
}

VkDeviceSize sb_bind_image(sb_device_arena *arena, VkDevice device, VkImage image)
{
	VK_CHECK(vkBindImageMemory(device, image, arena->memory, arena->offset));

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(device, image, &memory_requirements);
	return sb_offset_device_arena_aligned(arena, memory_requirements.size, memory_requirements.alignment);
}

void sb_reset_device_arena(sb_device_arena *arena)
{
	arena->offset = 0;
}

VkDeviceMemory sb_dedicated_image_allocation(const sb_memory_types *memory_types, VkDevice device, VkImage image)
{
	VkMemoryDedicatedAllocateInfo dedicated_memory_allocate_info = {0};
	dedicated_memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
	dedicated_memory_allocate_info.image = image;

	VkMemoryRequirements image_memory_requirements;
	vkGetImageMemoryRequirements(device, image, &image_memory_requirements);

	VkMemoryAllocateInfo memory_allocate_info = {0};
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.pNext = &dedicated_memory_allocate_info;
	memory_allocate_info.allocationSize = image_memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = sb_get_memory_index(memory_types, image_memory_requirements.memoryTypeBits, SB_MEMORY_USAGE_GPU);

	VkDeviceMemory memory;
	VK_CHECK(vkAllocateMemory(device, &memory_allocate_info, NULL, &memory));
	VK_CHECK(vkBindImageMemory(device, image, memory, 0));
	return memory;
}

VkBufferMemoryBarrier2 sb_get_buffer_barrier(sb_buffer *buffer)
{
	VkBufferMemoryBarrier2 buffer_barrier = {0};
	buffer_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
	buffer_barrier.buffer = buffer->vk_buffer;
	buffer_barrier.size = VK_WHOLE_SIZE;
	return buffer_barrier;
}

void sb_image_barriers(VkCommandBuffer command_buffer, VkImageMemoryBarrier2 *barriers, uint32_t count)
{
	VkDependencyInfo dependency_info = {0};
	dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependency_info.imageMemoryBarrierCount = count;
	dependency_info.pImageMemoryBarriers = barriers;
	vkCmdPipelineBarrier2(command_buffer, &dependency_info);
}

void sb_buffer_barriers(VkCommandBuffer command_buffer, VkBufferMemoryBarrier2 *barriers, uint32_t count)
{
	VkDependencyInfo dependency_info = {0};
	dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependency_info.bufferMemoryBarrierCount = count;
	dependency_info.pBufferMemoryBarriers = barriers;
	vkCmdPipelineBarrier2(command_buffer, &dependency_info);
}

void sb_buffer_copy(VkCommandBuffer command_buffer, const sb_buffer_copy_info *info)
{
	VkBufferCopy2 buffer_copy = {0};
	buffer_copy.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2;
	buffer_copy.size =  info->size;
	buffer_copy.srcOffset = info->src_offset;
	buffer_copy.dstOffset = info->dst_offset;
	 
	VkCopyBufferInfo2 copy_info = {0};
	copy_info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2;
	copy_info.srcBuffer = info->src_buffer->vk_buffer;
	copy_info.dstBuffer = info->dst_buffer->vk_buffer;
	copy_info.pRegions = &buffer_copy;
	copy_info.regionCount = 1;

	vkCmdCopyBuffer2(command_buffer, &copy_info);
}
