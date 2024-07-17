#ifndef SB_SWAPCHAIN_H
#define SB_SWAPCHAIN_H

#include <vulkan/vulkan.h>
#include <stdbool.h>

#define SB_SWAPCHAIN_IMAGE_USAGE (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)

VkPresentModeKHR sb_get_present_mode(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
VkFormat sb_get_swapchain_format(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

static uint32_t get_image_count(uint32_t min_image_count, uint32_t max_image_count);

VkSwapchainKHR sb_create_swapchain(VkDevice device, VkSurfaceKHR surface, VkPhysicalDevice physical_device, VkSwapchainKHR old_swapchain, 
	VkFormat format, VkPresentModeKHR present_mode);
    
int sb_acquire_next_image( VkDevice device, VkSwapchainKHR swapchain, VkSemaphore image_available);
bool sb_present_image(VkSwapchainKHR swapchain, VkQueue present_queue, VkSemaphore render_finished, int current_image);

#endif
