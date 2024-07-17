#include "sb_common.h"
#include "sb_swapchain.h"
#include "sb_arena.h"

#define DESIRED_SWAPCHAIN_IMAGES 3U

VkPresentModeKHR sb_get_present_mode(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
	uint32_t present_mode_count;
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, NULL));

    sb_arena_temp scratch = sb_get_scratch();
	VkPresentModeKHR *present_modes = sb_arena_push(scratch.arena, VkPresentModeKHR, present_mode_count);
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, present_modes));

	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
	for (uint32_t i = 0; i < present_mode_count; i++)
    {
		const VkPresentModeKHR current_present_mode = present_modes[i];

		if(current_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
		else if(current_present_mode == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
			present_mode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
	}

    sb_release_scratch(&scratch);
	return present_mode;
}

VkFormat sb_get_swapchain_format(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    uint32_t surface_format_count;
    // count > 0 by vulkan spec
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, NULL));

    sb_arena_temp scratch = sb_get_scratch();
	VkSurfaceFormatKHR *surface_formats = sb_arena_push(scratch.arena, VkSurfaceFormatKHR, surface_format_count);
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, surface_formats));

	VkFormat format = surface_formats[0].format;
	for (uint32_t i = 0; i < surface_format_count; i++)
    {
		if (surface_formats[i].format == VK_FORMAT_R8G8B8A8_SRGB)
        {
			format = surface_formats[i].format;
			break;
		}
	}

    sb_release_scratch(&scratch);
	return format; 
}

uint32_t get_image_count(uint32_t min_image_count, uint32_t max_image_count)
{
	if(min_image_count > DESIRED_SWAPCHAIN_IMAGES)
		return min_image_count;
	if(max_image_count != 0 && DESIRED_SWAPCHAIN_IMAGES > max_image_count)
		return max_image_count;
	return DESIRED_SWAPCHAIN_IMAGES;
}

VkSwapchainKHR sb_create_swapchain(VkDevice device, VkSurfaceKHR surface, VkPhysicalDevice physical_device, VkSwapchainKHR old_swapchain, 
	VkFormat format, VkPresentModeKHR present_mode)
{
	VkSurfaceCapabilitiesKHR surface_capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);

	VkSwapchainCreateInfoKHR swapchain_create_info = {0};
	swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_create_info.surface = surface;
	swapchain_create_info.minImageCount = get_image_count(surface_capabilities.minImageCount, surface_capabilities.maxImageCount);
	swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_create_info.imageFormat = format;
	swapchain_create_info.imageColorSpace =  VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	swapchain_create_info.imageExtent = surface_capabilities.currentExtent; // TODO: clamp
	swapchain_create_info.imageUsage = SB_SWAPCHAIN_IMAGE_USAGE;
	swapchain_create_info.imageArrayLayers = 1;
	swapchain_create_info.preTransform = surface_capabilities.currentTransform;
	swapchain_create_info.compositeAlpha = surface_capabilities.supportedCompositeAlpha;
	swapchain_create_info.clipped = VK_TRUE;
	swapchain_create_info.oldSwapchain = old_swapchain;
	swapchain_create_info.presentMode = present_mode;

	VkSwapchainKHR swapchain;
	VK_CHECK(vkCreateSwapchainKHR(device, &swapchain_create_info, NULL, &swapchain));
	return swapchain;
}

int sb_acquire_next_image( VkDevice device, VkSwapchainKHR swapchain, VkSemaphore image_available)
{
	int current_image;
	VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_available, VK_NULL_HANDLE, &current_image);
	switch (result)
	{
	case VK_SUCCESS: return current_image;
	case VK_ERROR_OUT_OF_DATE_KHR:
	case VK_SUBOPTIMAL_KHR: return -1;
	default: SB_PANIC("Swapchain failed to acquire next image!");
	}
}

bool sb_present_image(VkSwapchainKHR swapchain, VkQueue present_queue, VkSemaphore render_finished, int current_image)
{
	VkPresentInfoKHR present_info = {0};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &render_finished;
	present_info.pImageIndices = &current_image;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swapchain;

	VkResult result = vkQueuePresentKHR(present_queue, &present_info);
	switch (result)
	{
	case VK_SUCCESS: return true;
	case VK_ERROR_OUT_OF_DATE_KHR:
	case VK_SUBOPTIMAL_KHR: return false;
	default: SB_PANIC("Failed to present image!");
	}
}