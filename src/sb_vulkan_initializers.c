#include "sb_vulkan_initializers.h"
#include "sb_window.h"
#include "sb_arena.h"
#include "sb_file.h"
#include "sb_common.h"
#include "sb_texture.h"


#define MAX_QUEUE_CREATE_INFOS 2U

#define VK_B32_ARRAY(s) ((VkBool32*) (&((s)->pNext) + sizeof(void*)))
#define VK_B32_ARRAY_COUNT(type) ((sizeof(type) - (offsetof(type, pNext) + sizeof(void*)))/sizeof(VkBool32))

static const float QUEUE_PRIORITY = 1.0f;

static const char *ENABLED_DEVICE_EXTENSIONS[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

static const uint32_t ENABLED_DEVICE_EXTENSION_COUNT = COUNTOF(ENABLED_DEVICE_EXTENSIONS);

VkPhysicalDeviceVulkan13Features get_vulkan_13_features(void)
{
	VkPhysicalDeviceVulkan13Features features = {0};
	features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	features.synchronization2 = VK_TRUE;
	features.dynamicRendering = VK_TRUE;
	return features;
}

VkPhysicalDeviceVulkan12Features get_vulkan_12_features(void)
{
	VkPhysicalDeviceVulkan12Features features = {0};
	features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	features.runtimeDescriptorArray = VK_TRUE;
	features.descriptorBindingPartiallyBound = VK_TRUE;
	features.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
	features.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
	features.drawIndirectCount = VK_TRUE;
	features.bufferDeviceAddress = VK_TRUE;
	return features;
};

VkPhysicalDeviceFeatures get_vulkan_10_features(void)
{
	VkPhysicalDeviceFeatures features = {0};
	features.shaderInt64 = VK_TRUE;
	return features;
}
VkInstance sb_create_instance(const char *app_name)
{
	VkApplicationInfo application_info = {0};
	application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application_info.apiVersion = VK_API_VERSION_1_3;
	application_info.pApplicationName = app_name;
	application_info.pEngineName = "snowbound";
	application_info.applicationVersion = VK_API_VERSION_1_3;

	const char *instance_extensions[] = {
		VK_KHR_SURFACE_EXTENSION_NAME,
		SB_SURFACE_EXTENSION_NAME,
        #ifdef _DEBUG
		    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        #endif
	};

    #ifdef _DEBUG
	const char *enabled_layers[] = { "VK_LAYER_KHRONOS_validation", "VK_LAYER_KHRONOS_synchronization2"};
    #endif

	VkInstanceCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &application_info;
	create_info.enabledExtensionCount = COUNTOF(instance_extensions);
	create_info.ppEnabledExtensionNames = instance_extensions;
	#ifdef _DEBUG
	create_info.enabledLayerCount = COUNTOF(enabled_layers);
	create_info.ppEnabledLayerNames = enabled_layers;
	#endif

	VkInstance instance;
	VK_CHECK(vkCreateInstance(&create_info, NULL, &instance));
	return instance;
}

bool supports_extensions(VkPhysicalDevice physical_device)
{
    uint32_t device_extension_count;
	if (vkEnumerateDeviceExtensionProperties(physical_device, NULL, &device_extension_count, NULL) != VK_SUCCESS) return false;
	if (device_extension_count < 1) return false;

	sb_arena_temp scratch = sb_get_scratch();
	VkExtensionProperties *extensions = sb_arena_push(scratch.arena, VkExtensionProperties, device_extension_count);
	if (vkEnumerateDeviceExtensionProperties(physical_device, NULL, &device_extension_count, extensions) != VK_SUCCESS)
    {
        sb_release_scratch(&scratch);
        return false;
    }

	for (uint32_t j = 0; j < ENABLED_DEVICE_EXTENSION_COUNT; j++) {
		const char *enabled_extension_name = ENABLED_DEVICE_EXTENSIONS[j];

		bool found_extension = false;
		for (uint32_t i = 0; i < device_extension_count; i++)
        {
			const char *device_extension_name = extensions[i].extensionName;
			if (strcmp(enabled_extension_name, device_extension_name) != 0) continue;
			found_extension = true;
			break;
		}

		if (!found_extension)
        {
			sb_release_scratch(&scratch);
			return false;
		}
	}

	sb_release_scratch(&scratch);
    return true;
}

bool compare_features(const VkBool32 *supported, const VkBool32 *required, uint32_t count)
{
	for(int i = 0; i < count; i++)
	{
		VkBool32 is_supported = supported[i];
		VkBool32 is_required  = required[i];

		if(is_required && !is_supported) return false;
	}

	return true;
}

bool supports_features(VkPhysicalDevice physical_device)
{
	VkPhysicalDeviceVulkan13Features supported_vk_13_features = {0};
	supported_vk_13_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

	VkPhysicalDeviceVulkan12Features supported_vk_12_features = {0};
	supported_vk_12_features.pNext = &supported_vk_13_features;
	supported_vk_12_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

	VkPhysicalDeviceFeatures2 supported_features_2 = {0};
	supported_features_2.pNext  = &supported_vk_12_features;
	supported_features_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	
	vkGetPhysicalDeviceFeatures2(physical_device, &supported_features_2);

	VkPhysicalDeviceVulkan13Features required_vk_13_features = get_vulkan_13_features();
	VkPhysicalDeviceVulkan12Features required_vk_12_features = get_vulkan_12_features();
	VkPhysicalDeviceFeatures required_vk_10_features = get_vulkan_10_features();

	return compare_features(
		VK_B32_ARRAY(&supported_vk_12_features),
		VK_B32_ARRAY(&required_vk_12_features),
		VK_B32_ARRAY_COUNT(VkPhysicalDeviceVulkan12Features)
	) && compare_features(
		VK_B32_ARRAY(&supported_vk_13_features),
		VK_B32_ARRAY(&required_vk_13_features),
		VK_B32_ARRAY_COUNT(VkPhysicalDeviceVulkan13Features)
	) && compare_features(
		&supported_features_2.features, 
		&required_vk_10_features, 
		sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32)
	);
}

bool supports_queue_families(VkPhysicalDevice physical_device, VkSurfaceKHR surface,
	uint32_t *transfer_index, uint32_t *graphics_index)
{
	uint32_t queue_family_count;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, NULL);

	// family count guarunteed to be >= 1 by vulkan spec
    sb_arena_temp scratch = sb_get_scratch();
	VkQueueFamilyProperties *queue_families = sb_arena_push(scratch.arena, VkQueueFamilyProperties, queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families);

	*transfer_index = UINT32_MAX;
	*graphics_index = UINT32_MAX;

	for (uint32_t i = 0; i < queue_family_count; i++) {
		VkQueueFamilyProperties *current_family = &queue_families[i];
		
		VkBool32 is_present_supported = VK_FALSE;
		VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &is_present_supported));

		if (current_family->queueFlags & VK_QUEUE_GRAPHICS_BIT && is_present_supported)
			*graphics_index = i;
        else if(current_family->queueFlags & VK_QUEUE_TRANSFER_BIT)
			*transfer_index = i;

	}

	sb_release_scratch(&scratch);

	// if we didnt manage to get a dedicated transfer queue, the graphics queue will implicitly support transfer ops
	if(*graphics_index != UINT32_MAX)
	{
		*transfer_index = *graphics_index;
		return true;
	}

	return false;
}

VkPhysicalDevice sb_get_physical_device(VkInstance instance, VkSurfaceKHR surface,
	uint32_t *transfer_queue_index, uint32_t *graphics_queue_index)
{
    uint32_t physical_device_count;
	VK_CHECK(vkEnumeratePhysicalDevices(instance, &physical_device_count, NULL));
	assert(physical_device_count > 0);

    sb_arena_temp scratch = sb_get_scratch();
	VkPhysicalDevice *physical_devices = sb_arena_push(scratch.arena, VkPhysicalDevice, physical_device_count);
	VK_CHECK(vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices));

	VkPhysicalDevice selected_gpu = VK_NULL_HANDLE;
	for (uint32_t i = 0; i < physical_device_count; i++)
    {
		VkPhysicalDevice current_gpu = physical_devices[i];

		if (!supports_extensions(current_gpu)) continue;
		if (!supports_queue_families(current_gpu, surface, transfer_queue_index, graphics_queue_index)) continue;
		if (!supports_features(current_gpu)) continue;

		selected_gpu = current_gpu;

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(current_gpu, &properties);
		if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			break;
		
	}

    assert(selected_gpu);
    sb_release_scratch(&scratch);
	return selected_gpu;
}

VkDeviceQueueCreateInfo get_queue_create_info(uint32_t family_index)
{
	VkDeviceQueueCreateInfo queue_create_info = {0};
	queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_info.queueFamilyIndex = family_index;
	queue_create_info.queueCount = 1;
	queue_create_info.pQueuePriorities = &QUEUE_PRIORITY;
	return queue_create_info;
}

VkDevice sb_create_device(VkPhysicalDevice physical_device, uint32_t transfer_queue_index, uint32_t graphics_queue_index)
{	
	uint32_t queue_create_info_count = 0;
	VkDeviceQueueCreateInfo queue_create_infos[MAX_QUEUE_CREATE_INFOS];
	queue_create_infos[queue_create_info_count++] = get_queue_create_info(graphics_queue_index);
	if(graphics_queue_index != transfer_queue_index)
		queue_create_infos[queue_create_info_count++] = get_queue_create_info(transfer_queue_index);

	VkPhysicalDeviceVulkan13Features vk13_features = get_vulkan_13_features();
	VkPhysicalDeviceVulkan12Features vk12_features = get_vulkan_12_features();
	vk12_features.pNext = &vk13_features;
	VkPhysicalDeviceFeatures vk_features = get_vulkan_10_features();

	VkDeviceCreateInfo device_create_info = {0};
	device_create_info.pNext = &vk12_features;
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.queueCreateInfoCount = queue_create_info_count;
	device_create_info.pQueueCreateInfos = queue_create_infos;
	device_create_info.enabledExtensionCount = ENABLED_DEVICE_EXTENSION_COUNT;
	device_create_info.ppEnabledExtensionNames = ENABLED_DEVICE_EXTENSIONS;
	device_create_info.pEnabledFeatures = &vk_features;

	VkDevice device;
	VK_CHECK(vkCreateDevice(physical_device, &device_create_info, NULL, &device));
	return device;
}

VkImageMemoryBarrier2 sb_get_image_layout_transition_barrier(VkImageAspectFlags aspect_flags)
{
	VkImageMemoryBarrier2 barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.aspectMask = aspect_flags;
	return barrier;
}

VkCommandPool sb_create_command_pool(VkDevice device, uint32_t familyIndex)
{
	VkCommandPoolCreateInfo pool_create_info = {0};
	pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_create_info.queueFamilyIndex = familyIndex;

	VkCommandPool command_pool;
	VK_CHECK(vkCreateCommandPool(device, &pool_create_info, NULL, &command_pool));
	return command_pool;
}

void sb_reset_command_pool(VkDevice device, VkCommandPool command_pool)
{
	VK_CHECK(vkResetCommandPool(device, command_pool, 0));
}

void sb_create_command_buffers(VkDevice device, VkCommandPool command_pool, VkCommandBuffer *command_buffers, uint32_t command_buffer_count)
{
	VkCommandBufferAllocateInfo command_buffer_allocate_info = {0};
	command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_allocate_info.commandPool = command_pool;
	command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_allocate_info.commandBufferCount = command_buffer_count;
	VK_CHECK(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, command_buffers));
}

void sb_begin_command_buffer(VkCommandBuffer command_buffer, bool one_time_submit)
{
	VkCommandBufferBeginInfo begin_info = {0};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = one_time_submit ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;
	VK_CHECK(vkBeginCommandBuffer(command_buffer, &begin_info));
}

void sb_end_command_buffer(VkCommandBuffer command_buffer)
{
	VK_CHECK(vkEndCommandBuffer(command_buffer));
}

VkSemaphore sb_create_semaphore(VkDevice device)
{
	VkSemaphoreCreateInfo semaphore_create_info = {0};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkSemaphore semaphore;
	VK_CHECK(vkCreateSemaphore(device, &semaphore_create_info, NULL, &semaphore));
	return semaphore;
}

VkFence sb_create_fence(VkDevice device, bool should_create_signaled)
{
	VkFenceCreateInfo fence_create_info = {0};
	fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_create_info.flags = should_create_signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
	
	VkFence fence;
	VK_CHECK(vkCreateFence(device, &fence_create_info, NULL, &fence));
	return fence;
}

VkQueue sb_get_queue(VkDevice device, uint32_t family_index)
{
	VkQueue queue;
	vkGetDeviceQueue(device, family_index, 0, &queue);
	return queue;
}

VkShaderModule sb_create_shader_module(VkDevice device, const char *name)
{
	sb_arena_temp scratch = sb_get_scratch();

	uint64_t size;
	void *buffer = sb_read_file_binary(scratch.arena, name, &size);

	VkShaderModuleCreateInfo create_info = { 0 };
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = size;
	create_info.pCode = (uint32_t*) buffer;

	VkShaderModule module;
	VK_CHECK(vkCreateShaderModule(device, &create_info, NULL, &module));
	sb_release_scratch(&scratch);
	return module;
}

VkPipelineLayout sb_create_pipeline_layout(VkDevice device, VkDescriptorSetLayout set_layout)
{
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {0};
	pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.pSetLayouts = &set_layout;
	pipeline_layout_create_info.setLayoutCount = 1;

	VkPipelineLayout pipeline_layout;
	VK_CHECK(vkCreatePipelineLayout(device, &pipeline_layout_create_info, NULL, &pipeline_layout));
    return pipeline_layout;
}

VkDescriptorSet sb_allocate_descriptor_set(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout)
{
	VkDescriptorSetAllocateInfo allocate_info = {0};
	allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocate_info.descriptorPool = pool;
	allocate_info.descriptorSetCount = 1;
	allocate_info.pSetLayouts = &layout;

	VkDescriptorSet set;
	VK_CHECK(vkAllocateDescriptorSets(device, &allocate_info, &set));
	return set;
}

void sb_update_draw_info_buffer_descriptor(VkDevice device, VkDescriptorSet descriptor_set, sb_buffer *draw_info_buffer)
{
    VkDescriptorBufferInfo buffer_info = {0};
    buffer_info.buffer = draw_info_buffer->vk_buffer;
    buffer_info.offset = 0;
    buffer_info.range = VK_WHOLE_SIZE;

	VkWriteDescriptorSet write = {0};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptor_set;
    write.dstArrayElement = 0;
    write.descriptorCount = 1;
    write.dstBinding = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.pBufferInfo = &buffer_info;

	vkUpdateDescriptorSets(device, 1, &write, 0, NULL);
}

VkDescriptorPool sb_create_descriptor_pool(VkDevice device)
{
    VkDescriptorPoolSize pool_sizes[] = {
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, SB_MAX_TEXTURES},
	};

	VkDescriptorPoolCreateInfo pool_create_info = {0};
	pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_create_info.maxSets = 1;
	pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
	pool_create_info.poolSizeCount = COUNTOF(pool_sizes);
	pool_create_info.pPoolSizes = pool_sizes;

	VkDescriptorPool pool;
	VK_CHECK(vkCreateDescriptorPool(device, &pool_create_info, NULL, &pool));
    return pool;
}

VkImage sb_create_image(VkDevice device, VkExtent2D extent, VkFormat format, VkImageUsageFlags usage, VkSampleCountFlags samples)
{
	VkExtent3D image_extent = { 0 };
	image_extent.width = extent.width;
	image_extent.height = extent.height;
	image_extent.depth = 1;

	VkImageCreateInfo image_create_info = { 0 };
	image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.extent = image_extent;
	image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_create_info.imageType = VK_IMAGE_TYPE_2D;
	image_create_info.mipLevels = 1;
	image_create_info.usage = usage;//VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_create_info.arrayLayers = 1;
	image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_create_info.format = format;
	image_create_info.samples = samples;
	image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;

	VkImage texture_image;
	VK_CHECK(vkCreateImage(device, &image_create_info, NULL, &texture_image));
	return texture_image;
}

VkImageView sb_create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect_flag)
{
	VkImageViewCreateInfo view_create_info = { 0 };
	view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_create_info.image = image;
	view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_create_info.format = format;
	view_create_info.subresourceRange.aspectMask = aspect_flag;
	view_create_info.subresourceRange.baseMipLevel = 0;
	view_create_info.subresourceRange.levelCount = 1;
	view_create_info.subresourceRange.baseArrayLayer = 0;
	view_create_info.subresourceRange.layerCount = 1;

	VkImageView view;
	VK_CHECK(vkCreateImageView(device, &view_create_info, NULL, &view));
	return view;
}

VkSampler sb_create_sampler(VkDevice device, VkSamplerAddressMode address_mode,VkBorderColor border_color)
{
	VkSamplerCreateInfo sampler_info = {0};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_LINEAR;
	sampler_info.minFilter = VK_FILTER_LINEAR;
	sampler_info.addressModeU = address_mode;
	sampler_info.addressModeV = address_mode;
	sampler_info.addressModeW = address_mode;
	sampler_info.anisotropyEnable = VK_FALSE;
	sampler_info.maxAnisotropy = 1.0f;
	sampler_info.borderColor = border_color;;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.mipLodBias = 0.0f;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = 0.0f;

	VkSampler sampler;
	VK_CHECK(vkCreateSampler(device, &sampler_info, NULL, &sampler));
	return sampler;
}

void sb_queue_submit(VkQueue queue, const sb_queue_submit_info *queue_submit)
{
	VkSemaphoreSubmitInfo wait_info = {0};
	wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	wait_info.semaphore = queue_submit->wait_semaphore;
	wait_info.stageMask = queue_submit->wait_stage_mask;

	VkCommandBufferSubmitInfo command_buffer_info = {0};
	command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	command_buffer_info.commandBuffer = queue_submit->command_buffer;

	VkSemaphoreSubmitInfo signal_info = {0};
	signal_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	signal_info.semaphore = queue_submit->signal_semaphore;
	signal_info.stageMask = queue_submit->signal_stage_mask;

	VkSubmitInfo2 submit_info = {0};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submit_info.waitSemaphoreInfoCount = queue_submit->wait_semaphore ? 1 : 0;
	submit_info.pWaitSemaphoreInfos = &wait_info;
	submit_info.signalSemaphoreInfoCount = queue_submit->signal_semaphore ? 1 : 0;
	submit_info.pSignalSemaphoreInfos = &signal_info;
	submit_info.commandBufferInfoCount = 1;
	submit_info.pCommandBufferInfos = &command_buffer_info;

	VK_CHECK(vkQueueSubmit2(queue,1, &submit_info, queue_submit->fence));
}

void sb_wait_for_fence(VkDevice device, VkFence fence)
{
	VK_CHECK(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));
}

void sb_reset_fence(VkDevice device, VkFence fence)
{
	VK_CHECK(vkResetFences(device, 1, &fence));
}
