#ifndef SB_VULKAN_MEMORY_H
#define SB_VULKAN_MEMORY_H

#include <vulkan/vulkan.h>

typedef enum
{
	SB_MEMORY_USAGE_CPU = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	SB_MEMORY_USAGE_GPU = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	SB_MEMORY_USAGE_CPU_TO_GPU = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
} sb_memory_usage;

typedef struct
{
	uint32_t memory_type_count;
	VkMemoryType memory_types[VK_MAX_MEMORY_TYPES];
} sb_memory_types;

sb_memory_types sb_get_memory_types(VkPhysicalDevice physicalDevice);
uint32_t sb_get_memory_index(const sb_memory_types *memory_types,
	uint32_t memory_type_bits_requirement, sb_memory_usage memory_usage);

static VkBuffer create_vk_buffer(VkDevice device, VkDeviceSize capacity, VkBufferUsageFlags buffer_usage);
static VkDeviceMemory allocate_and_bind_memory(VkDevice device, VkBuffer buffer, sb_memory_usage memory_usage, const sb_memory_types *memory_types);
static VkMemoryPropertyFlags get_memory_properties(sb_memory_usage memory_usage);
static VkDeviceAddress get_address(VkDevice device, VkBuffer buffer);
static void *map_buffer(VkDevice device, VkBuffer buffer);

typedef struct
{
	VkDeviceSize capacity;
	sb_memory_usage memory_usage;
	VkBufferUsageFlags buffer_usage_flags;
	const sb_memory_types *memory_types;
} sb_memory_info;

#define DEFINE_BUFFER_FIELDS \
	VkBuffer vk_buffer;\
	VkDeviceMemory memory;\
	VkDeviceAddress address;\
	void *memory_ptr;\
	VkDeviceSize capacity;

typedef struct
{
	DEFINE_BUFFER_FIELDS
} sb_buffer;

typedef struct
{
	DEFINE_BUFFER_FIELDS
	VkDeviceSize offset;
} sb_device_arena;

#undef DEFINE_BUFFER_FIELDS

void sb_allocate_buffer(VkDevice device, const sb_memory_info *info, sb_buffer *out_buffer);
void sb_allocate_device_arena(VkDevice device, const sb_memory_info *info, sb_device_arena *out_arena);

typedef struct
{
	sb_device_arena *src;
	VkDeviceSize src_offset;
	sb_device_arena *dst;

	VkDeviceSize transfer_size;
	VkDeviceSize transfer_align;
} sb_memory_transfer_command;

VkDeviceSize sb_offset_device_arena_aligned(sb_device_arena *arena, VkDeviceSize size, VkDeviceSize align);
#define sb_offset_device_arena(arena, type, count) sb_offset_device_arena_aligned(arena, sizeof(type)*(count), _Alignof(type))

void *sb_get_ptr(sb_device_arena *arena, VkDeviceSize offset);
VkDeviceAddress sb_get_address(sb_device_arena *arena, VkDeviceSize offset);

VkDeviceSize sb_bind_image(sb_device_arena *arena, VkDevice device, VkImage image);
void sb_reset_device_arena(sb_device_arena *arena);

VkDeviceMemory sb_dedicated_image_allocation(const sb_memory_types *memory_types, VkDevice device, VkImage image);

VkBufferMemoryBarrier2 sb_get_buffer_barrier(sb_buffer *buffer);

void sb_buffer_barriers(VkCommandBuffer command_buffer, VkBufferMemoryBarrier2 *barriers, uint32_t count);
void sb_image_barriers(VkCommandBuffer command_buffer, VkImageMemoryBarrier2 *barriers, uint32_t count);

typedef struct 
{
	sb_buffer *src_buffer;
	VkDeviceSize src_offset;

	sb_buffer *dst_buffer;
	VkDeviceSize dst_offset;

	VkDeviceSize size;
} sb_buffer_copy_info;

void sb_buffer_copy(VkCommandBuffer command_buffer, const sb_buffer_copy_info *info);

#endif
