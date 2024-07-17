#ifndef SB_VULKAN_INITIALIZERS_H
#define SB_VULKAN_INITIALIZERS_H

#include <vulkan/vulkan.h>
#include <stdbool.h>

#include "sb_vulkan_memory.h"

typedef struct
{
	VkFence fence;
	
	VkSemaphore wait_semaphore;
	VkPipelineStageFlags2 wait_stage_mask;

	VkSemaphore signal_semaphore;
	VkPipelineStageFlags2 signal_stage_mask;

	VkCommandBuffer command_buffer;
} sb_queue_submit_info;

void sb_queue_submit(VkQueue queue, const sb_queue_submit_info *queue_submit);

static VkPhysicalDeviceVulkan13Features get_vulkan_13_features(void);
static VkPhysicalDeviceVulkan12Features get_vulkan_12_features(void);
static VkPhysicalDeviceFeatures get_vulkan_10_features(void);

VkInstance sb_create_instance(const char *app_name);

static bool supports_extensions(VkPhysicalDevice physicalDevice);
static bool supports_queue_families(VkPhysicalDevice physical_device, VkSurfaceKHR surface,
	uint32_t *transfer_index, uint32_t *graphics_index);
static bool compare_features(VkBool32 *supported, VkBool32 *required, uint32_t size);
static bool supports_features(VkPhysicalDevice physical_device);

VkPresentModeKHR sb_get_present_mode(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
VkFormat sb_get_swapchain_format(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
VkPhysicalDevice sb_get_physical_device(VkInstance instance, VkSurfaceKHR surface,
	uint32_t *transfer_queue_index, uint32_t *graphics_queue_index);

static VkDeviceQueueCreateInfo get_queue_create_info(uint32_t family_index);

VkDevice sb_create_device(VkPhysicalDevice physical_device, uint32_t transfer_queue_index, uint32_t graphics_queue_index);
VkSwapchainKHR sb_create_swapchain(VkDevice device, VkSurfaceKHR surface, VkPhysicalDevice physical_device, VkSwapchainKHR old_swapchain,
	VkFormat format, VkPresentModeKHR present_mode);
VkImageMemoryBarrier2 sb_get_image_layout_transition_barrier(VkImageAspectFlags aspect_flags);
VkQueue sb_get_queue(VkDevice device, uint32_t family_index);

VkCommandPool sb_create_command_pool(VkDevice device, uint32_t familyIndex);
void sb_reset_command_pool(VkDevice device, VkCommandPool command_pool);
void sb_create_command_buffers(VkDevice device, VkCommandPool command_pool, VkCommandBuffer *command_buffers, uint32_t command_buffer_count);

void sb_begin_command_buffer(VkCommandBuffer command_buffer, bool one_time_submit);
void sb_end_command_buffer(VkCommandBuffer command_buffer);

VkSemaphore sb_create_semaphore(VkDevice device);
VkFence sb_create_fence(VkDevice device, bool should_create_signaled);

VkShaderModule sb_create_shader_module(VkDevice device, const char *name);
VkPipelineLayout sb_create_pipeline_layout(VkDevice device, VkDescriptorSetLayout set_layout);
VkDescriptorSet sb_allocate_descriptor_set(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout);

void sb_update_draw_info_buffer_descriptor(VkDevice device, VkDescriptorSet descriptor_set, sb_buffer *draw_info_buffer);
VkDescriptorPool sb_create_descriptor_pool(VkDevice device);

VkImage sb_create_image(VkDevice device, VkExtent2D extent, VkFormat format, VkImageUsageFlags usage, VkSampleCountFlags samples);
VkImageView sb_create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect_flag);
VkSampler sb_create_sampler(VkDevice device, VkSamplerAddressMode address_mode,VkBorderColor border_color);

void sb_wait_for_fence(VkDevice device, VkFence fence);
void sb_reset_fence(VkDevice device, VkFence fence);



#endif
