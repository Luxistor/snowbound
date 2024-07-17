#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "sb_app.h"
#include "sb_common.h"
#include "sb_arena.h"
#include "sb_file.h"
#include "sb_vulkan_initializers.h"
#include "sb_swapchain.h"

VkSpecializationInfo *get_specialization_info(sb_arena *arena, const VkDeviceAddress *addresses, uint32_t address_count)
{
    VkSpecializationMapEntry *entries = sb_arena_push(arena, VkSpecializationMapEntry, address_count);
    for(int i = 0; i < address_count; i++)
    {
        VkSpecializationMapEntry *entry = &entries[i];
        entry->constantID = i;
        entry->offset = i*sizeof(VkDeviceAddress);
        entry->size = sizeof(VkDeviceAddress);
    }

    VkSpecializationInfo *specialization = sb_arena_one(arena, VkSpecializationInfo);
    specialization->dataSize = sizeof(VkDeviceAddress)*address_count;
    specialization->mapEntryCount = address_count;
    specialization->pMapEntries = entries;
    specialization->pData = (const void*)addresses;
    return specialization;
}

VkPipeline sb_create_graphics_pipeline(sb_app *app, const sb_graphics_pipeline_info *pipeline_info)
{
    VkVertexInputBindingDescription vertex_binding = {0};
	vertex_binding.binding = 0;
	vertex_binding.stride = sizeof(sb_vertex);
	vertex_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription position_attribute = {0};
	position_attribute.location = 0;
	position_attribute.binding = 0;
	position_attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	position_attribute.offset = offsetof(sb_vertex, position);

	VkVertexInputAttributeDescription normal_attribute = {0};
	normal_attribute.location = 1;
	normal_attribute.binding = 0;
	normal_attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	normal_attribute.offset = offsetof(sb_vertex, normal);

	VkVertexInputAttributeDescription uv_attribute = {0};
	uv_attribute.location = 2;
	uv_attribute.binding = 0;
	uv_attribute.format = VK_FORMAT_R32G32_SFLOAT;
	uv_attribute.offset = offsetof(sb_vertex, uv);

	VkVertexInputAttributeDescription attributes[] = { position_attribute, normal_attribute, uv_attribute };

    const char *vertex_shader_name = "shaders/spv/fullscreen_tri.spv";

	VkPipelineVertexInputStateCreateInfo vertex_input_state = {0};
	vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    if(pipeline_info->vertex_shader_name)
    {
        vertex_shader_name = pipeline_info->vertex_shader_name;

        vertex_input_state.vertexAttributeDescriptionCount = COUNTOF(attributes);
        vertex_input_state.pVertexAttributeDescriptions = attributes;
        vertex_input_state.vertexBindingDescriptionCount = 1;
        vertex_input_state.pVertexBindingDescriptions = &vertex_binding;
    }

    sb_arena_temp scratch = sb_get_scratch();

	VkShaderModule vertex_shader_module = sb_create_shader_module(app->device, vertex_shader_name);
	VkPipelineShaderStageCreateInfo vertex_shader = {0};
	vertex_shader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertex_shader.module = vertex_shader_module;
	vertex_shader.stage =  VK_SHADER_STAGE_VERTEX_BIT;
    vertex_shader.pName = "main";
    if(pipeline_info->vertex_ubo_count > 0)
    {
        vertex_shader.pSpecializationInfo =
            get_specialization_info(scratch.arena, pipeline_info->vertex_ubos, pipeline_info->vertex_ubo_count);
    }

    VkShaderModule frag_shader_module = VK_NULL_HANDLE;
    VkPipelineShaderStageCreateInfo frag_shader = {0};
    if(pipeline_info->fragment_shader_name)
    {
        frag_shader_module = sb_create_shader_module(app->device, pipeline_info->fragment_shader_name);

        frag_shader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_shader.module = frag_shader_module;
        frag_shader.stage =  VK_SHADER_STAGE_FRAGMENT_BIT;
        if(pipeline_info->fragment_ubos_count > 0)
        {
            frag_shader.pSpecializationInfo =
                get_specialization_info(scratch.arena, pipeline_info->fragment_ubos, pipeline_info->fragment_ubos_count);
        }
        frag_shader.pName = "main";
    }

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {0};
	input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_state.primitiveRestartEnable = false;

	VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamic_state = {0};
	dynamic_state.dynamicStateCount = COUNTOF(dynamic_states);
	dynamic_state.pDynamicStates = dynamic_states;
	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

	VkPipelineViewportStateCreateInfo viewport_state = {0};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.scissorCount = 1;
	viewport_state.viewportCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterization_state = {0};
	rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization_state.cullMode = pipeline_info->cull_enabled ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
	rasterization_state.lineWidth = 1.0f;
	rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    if(pipeline_info->depth_bias)
    {
        rasterization_state.depthBiasEnable = VK_TRUE;
        rasterization_state.depthBiasSlopeFactor = pipeline_info->depth_bias->slope_factor;
        rasterization_state.depthBiasConstantFactor = pipeline_info->depth_bias->constant_factor;
    }

	VkPipelineMultisampleStateCreateInfo multisample_state = {0};
	multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {0};
	depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil_state.depthTestEnable = VK_TRUE;
	depth_stencil_state.depthWriteEnable = VK_TRUE;
	depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depth_stencil_state.maxDepthBounds = 1.0f;
    depth_stencil_state.minDepthBounds = 0.0f;
	depth_stencil_state.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState *color_blend_attachments = sb_arena_push(scratch.arena, VkPipelineColorBlendAttachmentState, pipeline_info->color_attachment_count);
    for(int i = 0; i < pipeline_info->color_attachment_count; i++)
    {
	    VkPipelineColorBlendAttachmentState *color_blend_attachment = &color_blend_attachments[i];
	    color_blend_attachment->srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	    color_blend_attachment->dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	    color_blend_attachment->colorBlendOp = VK_BLEND_OP_ADD;
	    color_blend_attachment->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	    color_blend_attachment->dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	    color_blend_attachment->alphaBlendOp = VK_BLEND_OP_ADD;
	    color_blend_attachment->colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    }

	VkPipelineColorBlendStateCreateInfo color_blend_state = {0};
	color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_state.logicOp = VK_LOGIC_OP_COPY;
	color_blend_state.attachmentCount = pipeline_info->color_attachment_count;
	color_blend_state.pAttachments = color_blend_attachments;

    VkPipelineRenderingCreateInfo pipeline_rendering_create_info = {0};
	pipeline_rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	pipeline_rendering_create_info.colorAttachmentCount = pipeline_info->color_attachment_count;
    pipeline_rendering_create_info.pColorAttachmentFormats = pipeline_info->color_attachment_formats;
    pipeline_rendering_create_info.depthAttachmentFormat = pipeline_info->depth_attachment_format;

    VkPipelineShaderStageCreateInfo shaders[2] = {vertex_shader, frag_shader};
	VkGraphicsPipelineCreateInfo pipeline_create_info = {0};
	pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.pNext = &pipeline_rendering_create_info;
	pipeline_create_info.stageCount = frag_shader_module ? 2 : 1;
	pipeline_create_info.pStages = shaders;
	pipeline_create_info.pVertexInputState = &vertex_input_state;
	pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	pipeline_create_info.pViewportState = &viewport_state;
	pipeline_create_info.pRasterizationState = &rasterization_state;
	pipeline_create_info.pMultisampleState = &multisample_state;
	pipeline_create_info.pDynamicState = &dynamic_state;
	pipeline_create_info.pDepthStencilState = &depth_stencil_state;
	pipeline_create_info.pColorBlendState = &color_blend_state;
	pipeline_create_info.layout = app->global_pipeline_layout;
	pipeline_create_info.renderPass = VK_NULL_HANDLE;
	pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	pipeline_create_info.basePipelineIndex = -1;

	VkPipeline pipeline;
	VK_CHECK(vkCreateGraphicsPipelines(app->device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &pipeline));

    if(frag_shader_module)
        vkDestroyShaderModule(app->device, frag_shader_module, NULL);
    vkDestroyShaderModule(app->device, vertex_shader_module, NULL);

    sb_release_scratch(&scratch);

    return pipeline;
}


VkPipeline sb_create_compute_pipeline(sb_app *app, const sb_compute_pipeline_info *pipeline_info)
{
    sb_arena_temp scratch = sb_get_scratch();
    VkShaderModule comp_shader_module = sb_create_shader_module(app->device, pipeline_info->compute_shader_name);

	VkPipelineShaderStageCreateInfo comp_shader = {0};
	comp_shader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	comp_shader.module = comp_shader_module;
	comp_shader.stage =  VK_SHADER_STAGE_COMPUTE_BIT;
    if(pipeline_info->address_count > 0)
    {
        comp_shader.pSpecializationInfo =
            get_specialization_info(scratch.arena, pipeline_info->addresses, pipeline_info->address_count);

    }
	comp_shader.pName = "main";

	VkComputePipelineCreateInfo pipeline_create_info = {0};
	pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipeline_create_info.layout = app->global_pipeline_layout;
	pipeline_create_info.stage = comp_shader;
	pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	pipeline_create_info.basePipelineIndex = -1;

	VkPipeline pipeline;
	VK_CHECK(vkCreateComputePipelines(app->device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &pipeline));
	vkDestroyShaderModule(app->device, comp_shader_module, NULL);

    sb_release_scratch(&scratch);
    return pipeline;
}

VkImageAspectFlags sb_get_aspect_flag(sb_texture_type type)
{
    return type == SB_TEXTURE_TYPE_DEPTH ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
}

sb_texture_id sb_create_texture(sb_app *app, const sb_texture_info *info)
{
    sb_texture_id id = ++app->texture_count;
    sb_texture *texture = sb_get_texture(app, id);

    VkImageUsageFlags image_usage = 0;
    if(info->usage & SB_TEXTURE_USAGE_SHADER_READ_FLAG)
    {
        texture->sampler = sb_create_sampler(app->device, info->sampler_address_mode, info->sampler_border_color);
        image_usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        app->texture_descriptor_updates[app->texture_descriptor_update_count++] = id;
    }

    if(info->usage & SB_TEXTURE_USAGE_RENDER_ATTACHMENT_FLAG)
        image_usage |= info->texture_type == SB_TEXTURE_TYPE_DEPTH ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if(info->usage & SB_TEXTURE_USAGE_TRANSFER_DST)
        image_usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    VkExtent2D extent = info->extent;
    if(info->flags & SB_TEXTURE_FLAG_WINDOW_RELATIVE)
    {
        texture->is_window_relative = true;
        extent = sb_get_window_extent(app->window);
    }

    texture->image = sb_create_image(app->device, extent, info->format, image_usage, VK_SAMPLE_COUNT_1_BIT);
    if(info->flags & SB_TEXTURE_FLAG_DEDICATED_ALLOCATION)
        texture->memory = sb_dedicated_image_allocation(&app->memory_types, app->device, texture->image);
    else
        texture->offset = sb_bind_image(&app->image_arena, app->device, texture->image);

    texture->format = info->format;
    texture->image_usage = image_usage;
    texture->texture_type = info->texture_type;
    texture->extent = extent;
    texture->view = sb_create_image_view(app->device, texture->image, info->format, sb_get_aspect_flag(info->texture_type));

    return id;
}

sb_texture_id sb_texture_from_file(sb_app *app, const char *file_path)
{
	int tex_width, tex_height, tex_channels;
	char *pixels = stbi_load(file_path, &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
	assert(pixels);

    sb_texture_info info = {0};
    info.extent = (VkExtent2D) {tex_width, tex_height};
    info.format = VK_FORMAT_R8G8B8A8_SRGB;
    info.texture_type = SB_TEXTURE_TYPE_COLOR;
    info.sampler_address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.sampler_border_color = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    info.usage = SB_TEXTURE_USAGE_SHADER_READ_FLAG | SB_TEXTURE_USAGE_TRANSFER_DST;
    sb_texture_id id = sb_create_texture(app, &info);

    sb_texture_transfer *transfer = sb_queue_texture_transfer(&app->transfer_buffer);
    transfer->texture_id = id;
    transfer->pixels = pixels;

	return id;
}

sb_texture *sb_get_texture(sb_app *app, sb_texture_id id)
{
    return &app->texture_handles[id];
}

void *sb_alloc_ubo(sb_app *app, VkDeviceSize size, VkDeviceAddress *out_address)
{
    VkDeviceSize offset = sb_offset_device_arena_aligned(&app->ubo_staging_arena, size, 16);

    *out_address = sb_get_address(&app->ubo_gpu_memory, offset);
    return sb_get_ptr(&app->ubo_staging_arena, offset);

}

VkAccessFlags2 get_access_mask(VkImageLayout layout, sb_texture_type texture_type)
{
    switch(layout)
    {
        case SB_IMAGE_LAYOUT_PRESENT: return 0;
        case SB_IMAGE_LAYOUT_UNDEFINED: return 0;
        case SB_IMAGE_LAYOUT_RENDER_ATTACHMENT:return texture_type == SB_TEXTURE_TYPE_DEPTH ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        case SB_IMAGE_LAYOUT_READ_ONLY: return VK_ACCESS_SHADER_READ_BIT;
    }
}

// for images specifically used in rendering
void sb_set_image_layouts(VkCommandBuffer command_buffer, const sb_image_transition *texture_barriers, uint32_t texture_barrier_count)
{
    sb_arena_temp scratch = sb_get_scratch();

    uint32_t image_barrier_count = 0;
    VkImageMemoryBarrier2 *image_barriers = sb_arena_push(scratch.arena, VkImageMemoryBarrier2, texture_barrier_count*2);
    for(int i = 0; i  < texture_barrier_count; i++)
    {
        const sb_image_transition *texture_barrier = &texture_barriers[i];

        VkImageMemoryBarrier2 *image_barrier = &image_barriers[image_barrier_count++];
        image_barrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        image_barrier->oldLayout = texture_barrier->old_layout;
        image_barrier->newLayout = texture_barrier->new_layout;
        image_barrier->srcStageMask = texture_barrier->src_stage_mask;
        image_barrier->dstStageMask = texture_barrier->dst_stage_mask;



        image_barrier->image = texture_barrier->texture->image;
        image_barrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_barrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_barrier->subresourceRange.baseArrayLayer = 0;
        image_barrier->subresourceRange.layerCount = 1;
        image_barrier->subresourceRange.levelCount = 1;
        image_barrier->subresourceRange.baseMipLevel = 0;
        image_barrier->subresourceRange.aspectMask = sb_get_aspect_flag(texture_barrier->texture->texture_type);

        image_barrier->srcAccessMask = get_access_mask(texture_barrier->old_layout, texture_barrier->texture->texture_type);
        image_barrier->dstAccessMask = get_access_mask(texture_barrier->new_layout, texture_barrier->texture->texture_type);

    }

    sb_image_barriers(command_buffer, image_barriers, image_barrier_count);
    sb_release_scratch(&scratch);
}


VkRenderingAttachmentInfo sb_rendering_attachment_info(sb_texture *texture, sb_clear_value clear_value)
{
    VkRenderingAttachmentInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    info.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    info.imageView = texture->view;
    if(texture->texture_type == SB_TEXTURE_TYPE_DEPTH)
        info.clearValue.depthStencil.depth = clear_value.depth;
    else
    {
        sb_vec4 color = {clear_value.color.r/255, clear_value.color.g/255, clear_value.color.b/255, 1};
        memcpy(&info.clearValue.color.float32, &color, sizeof(sb_vec4));
    }
    return info;
}

void sb_begin_render_pass(VkCommandBuffer command_buffer, sb_render_pass_info *info)
{
    VkRenderingInfo render_info = {0};
    render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    render_info.renderArea = (VkRect2D){ {0,0}, info->render_area};
    render_info.layerCount = 1;
    render_info.colorAttachmentCount = info->color_attachment_count;
    render_info.pColorAttachments = info->color_attachments;
    render_info.pDepthAttachment = info->depth_attachment;
    vkCmdBeginRendering(command_buffer, &render_info);

    VkViewport viewport = { 0 };
	viewport.x = 0;
	viewport.y = 0;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	viewport.width = info->render_area.width;
	viewport.height = info->render_area.height;
	vkCmdSetViewport(command_buffer, 0, 1, &viewport);

	VkRect2D scissor = { 0 };
	scissor.offset = (VkOffset2D){ 0,0 };
	scissor.extent = info->render_area;
	vkCmdSetScissor(command_buffer, 0, 1, &scissor);
}

void sb_draw_scene(VkCommandBuffer command_buffer, sb_buffer *draw_command_buffer)
{
    vkCmdDrawIndexedIndirectCount(
		command_buffer,
		draw_command_buffer->vk_buffer,
		offsetof(sb_indirect_command_array, array),
		draw_command_buffer->vk_buffer,
		offsetof(sb_indirect_command_array, count),
		SB_MAX_DRAW_COUNT,
		sizeof(VkDrawIndexedIndirectCommand)
	);
}
void sb_draw_pixels(VkCommandBuffer command_buffer)
{
    vkCmdDraw(command_buffer,3, 1, 0, 0);
}

VkDescriptorSetLayout create_global_set_layout(VkDevice device)
{
    #define DRAW_INFO_BUFFER_BINDING 0U
    VkDescriptorBindingFlags draw_info_binding_flags = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
    VkDescriptorSetLayoutBinding draw_info_binding = {0};
    draw_info_binding.binding = DRAW_INFO_BUFFER_BINDING;
    draw_info_binding.descriptorCount = 1;
    draw_info_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    draw_info_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;


    #define TEXTURE_ARRAY_BINDING 1U
    VkDescriptorBindingFlags texture_array_binding_flags = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
    VkDescriptorSetLayoutBinding texture_array_binding = {0};
    texture_array_binding.binding = TEXTURE_ARRAY_BINDING;
    texture_array_binding.descriptorCount = SB_MAX_TEXTURES;
    texture_array_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texture_array_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // debug
    #define MESH_HANDLE_ARRAY_BINDING 2U
    VkDescriptorBindingFlags mesh_handle_array_binding_flags = 0;
    VkDescriptorSetLayoutBinding mesh_handle_array_binding = {0};
    mesh_handle_array_binding.binding = MESH_HANDLE_ARRAY_BINDING;
    mesh_handle_array_binding.descriptorCount = 1;
    mesh_handle_array_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    mesh_handle_array_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding bindings[] = {draw_info_binding, texture_array_binding, mesh_handle_array_binding};
    VkDescriptorBindingFlags binding_flags[] = {draw_info_binding_flags, texture_array_binding_flags, mesh_handle_array_binding_flags};

    VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_info = {0};
	binding_flags_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	binding_flags_info.bindingCount = COUNTOF(binding_flags);
	binding_flags_info.pBindingFlags = binding_flags;

	VkDescriptorSetLayoutCreateInfo global_set_layout_create_info = {0};
	global_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    global_set_layout_create_info.pNext = &binding_flags_info;
	global_set_layout_create_info.bindingCount = COUNTOF(bindings);
	global_set_layout_create_info.pBindings = bindings;
    global_set_layout_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

    VkDescriptorSetLayout set_layout;
	VK_CHECK(vkCreateDescriptorSetLayout(device, &global_set_layout_create_info, NULL, &set_layout));
    return set_layout;
}

sb_app *sb_create_app(const sb_app_info *info)
{
    sb_app *app = malloc(sizeof(sb_app));
    SB_ZERO_STRUCT(app);

    app->bake_command_buffer = info->bake_command_buffer;

    app->window = sb_create_window(info->window_width, info->window_height, info->name);
    app->instance = sb_create_instance(info->name);
    app->surface = sb_create_surface(app->window, app->instance);

    uint32_t transfer_queue_index; uint32_t graphics_queue_index;
    app->physical_device = sb_get_physical_device(app->instance, app->surface, &transfer_queue_index, &graphics_queue_index);
    app->device = sb_create_device(app->physical_device, transfer_queue_index, graphics_queue_index);

    app->memory_types = sb_get_memory_types(app->physical_device);

    app->descriptor_pool = sb_create_descriptor_pool(app->device);
    app->global_set_layout = create_global_set_layout(app->device);
    app->global_set = sb_allocate_descriptor_set(app->device, app->descriptor_pool, app->global_set_layout);

    app->graphics_queue = sb_get_queue(app->device, graphics_queue_index);
    app->image_available_semaphore = sb_create_semaphore(app->device);
    app->render_finished_semaphore = sb_create_semaphore(app->device);
    app->render_finished_fence = sb_create_fence(app->device, true);

    app->global_pipeline_layout = sb_create_pipeline_layout(app->device, app->global_set_layout);
    app->swapchain_image_format = sb_get_swapchain_format(app->physical_device, app->surface);
    app->present_mode = sb_get_present_mode(app->physical_device, app->surface);
    app->swapchain = sb_create_swapchain(app->device, app->surface, app->physical_device, VK_NULL_HANDLE, app->swapchain_image_format, app->present_mode);
    app->command_pool = sb_create_command_pool(app->device, graphics_queue_index);
    app->swapchain_arena = sb_arena_alloc();
    app->mesh_memory = sb_alloc_mesh_memory(app->device, &app->memory_types);
    app->transfer_buffer = sb_create_transfer_buffer(app->device, &app->memory_types, transfer_queue_index, graphics_queue_index);

    sb_memory_info image_arena_info = {0};
    image_arena_info.capacity = MB(256);
    image_arena_info.memory_usage = SB_MEMORY_USAGE_GPU;
    image_arena_info.buffer_usage_flags = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    image_arena_info.memory_types = &app->memory_types;
    sb_allocate_device_arena(app->device, &image_arena_info, &app->image_arena);

    VkDeviceSize ubo_memory_size = MB(16);

    sb_memory_info ubo_staging_arena_info = {0};
    ubo_staging_arena_info.capacity = ubo_memory_size;
    ubo_staging_arena_info.memory_usage = SB_MEMORY_USAGE_CPU;
    ubo_staging_arena_info.buffer_usage_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    ubo_staging_arena_info.memory_types = &app->memory_types;
    sb_allocate_device_arena(app->device, &ubo_staging_arena_info, &app->ubo_staging_arena);

    sb_memory_info ubo_gpu_memory_info = {0};
    ubo_gpu_memory_info.capacity = ubo_memory_size;
    ubo_gpu_memory_info.memory_usage = SB_MEMORY_USAGE_GPU;
    ubo_gpu_memory_info.buffer_usage_flags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    ubo_gpu_memory_info.memory_types = &app->memory_types;
    sb_allocate_buffer(app->device, &ubo_gpu_memory_info, &app->ubo_gpu_memory);

    sb_memory_info indirect_command_buffer_info = {0};
    indirect_command_buffer_info.capacity = sizeof(sb_indirect_command_array);
    indirect_command_buffer_info.memory_usage = SB_MEMORY_USAGE_GPU;
    indirect_command_buffer_info.buffer_usage_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    indirect_command_buffer_info.memory_types = &app->memory_types;
    sb_allocate_buffer(app->device, &indirect_command_buffer_info, &app->indirect_command_buffer);

    for(int i = 0; i < 2; i++)
    {
        sb_memory_info draw_info_buffer_info = {0};
        draw_info_buffer_info.capacity = sizeof(sb_draw_info_array);
        draw_info_buffer_info.memory_usage = SB_MEMORY_USAGE_CPU_TO_GPU;
        draw_info_buffer_info.buffer_usage_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        draw_info_buffer_info.memory_types = &app->memory_types;
        sb_allocate_buffer(app->device, &draw_info_buffer_info, &app->draw_info_buffers[i]);
    }

    VkDeviceAddress addresses[2];
    addresses[0] = app->indirect_command_buffer.address;
    addresses[1] = app->mesh_memory.handle_buffer.address;

    sb_compute_pipeline_info cull_info = {0};
    cull_info.addresses = addresses;
    cull_info.address_count = 2;
    cull_info.compute_shader_name = "shaders/spv/comp.spv";

    app->compute_cull_pipeline = sb_create_compute_pipeline(app, &cull_info);

    VkDescriptorBufferInfo buffer_info = {0};
    buffer_info.buffer = app->mesh_memory.handle_buffer.vk_buffer;
    buffer_info.offset = 0;
    buffer_info.range = VK_WHOLE_SIZE;

	VkWriteDescriptorSet write = {0};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = app->global_set;
    write.dstArrayElement = 0;
    write.descriptorCount = 1;
    write.dstBinding = 2;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.pBufferInfo = &buffer_info;

	vkUpdateDescriptorSets(app->device, 1, &write, 0, NULL);

    return app;
}

void sb_recreate_swapchain(sb_app *app)
{
    for(int i = 0; i < app->image_count; i++)
    {
        vkDestroyImageView(app->device, app->image_views[i], NULL);
    }

    sb_reset_arena(app->swapchain_arena);

    VkSwapchainKHR old_swapchain = app->swapchain;

    app->swapchain = sb_create_swapchain(
        app->device,
        app->surface,
        app->physical_device,
        old_swapchain,
        app->swapchain_image_format,
        app->present_mode
    );

    vkDestroySwapchainKHR(app->device, old_swapchain, NULL);

    VK_CHECK(vkGetSwapchainImagesKHR(app->device, app->swapchain, &app->image_count, NULL));
	app->images = sb_arena_push(app->swapchain_arena, VkImage, app->image_count);
	VK_CHECK(vkGetSwapchainImagesKHR(app->device, app->swapchain, &app->image_count, app->images));

    app->command_buffers = sb_arena_push(app->swapchain_arena, VkCommandBuffer, app->image_count);

    app->image_views = sb_arena_push(app->swapchain_arena, VkImageView, app->image_count);
    for(int i = 0; i < app->image_count; i++)
        app->image_views[i] = sb_create_image_view(app->device, app->images[i], app->swapchain_image_format, VK_IMAGE_ASPECT_COLOR_BIT);
}

void sb_recreate_command_buffers(sb_app *app)
{
    sb_reset_command_pool(app->device, app->command_pool);
    sb_create_command_buffers(app->device, app->command_pool, app->command_buffers, app->image_count);

    for(int image_index = 0; image_index < app->image_count; image_index++)
    {
        VkCommandBuffer command_buffer = app->command_buffers[image_index];
        VkImage image = app->images[image_index];
        VkImageView image_view = app->image_views[image_index];

        sb_begin_command_buffer(command_buffer, false);
        sb_bind_mesh_buffers(command_buffer, &app->mesh_memory);

        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, app->global_pipeline_layout, 0, 1, &app->global_set, 0, NULL);
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->global_pipeline_layout, 0, 1, &app->global_set, 0, NULL);

        VkBufferMemoryBarrier2 pre_ubo_copy_barrier = sb_get_buffer_barrier(&app->ubo_gpu_memory);
        pre_ubo_copy_barrier.srcAccessMask = 0;
        pre_ubo_copy_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        pre_ubo_copy_barrier.srcStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        pre_ubo_copy_barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        sb_buffer_barriers(command_buffer, &pre_ubo_copy_barrier, 1);

        sb_buffer_copy_info ubo_copy = {0};
        ubo_copy.src_buffer = &app->ubo_staging_arena;
        ubo_copy.dst_buffer = &app->ubo_gpu_memory;
        ubo_copy.size = app->ubo_staging_arena.offset;
        sb_buffer_copy(command_buffer, &ubo_copy);

        VkBufferMemoryBarrier2 post_ubo_copy_barrier = sb_get_buffer_barrier(&app->ubo_gpu_memory);
        post_ubo_copy_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        post_ubo_copy_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        post_ubo_copy_barrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        post_ubo_copy_barrier.dstStageMask = VK_SHADER_STAGE_VERTEX_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        VkBufferMemoryBarrier2 prefill_barrier = sb_get_buffer_barrier(&app->indirect_command_buffer);
        prefill_barrier.srcStageMask = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        prefill_barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        prefill_barrier.srcAccessMask = 0;
        prefill_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        VkBufferMemoryBarrier2 barriers[] = {post_ubo_copy_barrier, prefill_barrier};
        sb_buffer_barriers(command_buffer, barriers, COUNTOF(barriers));

        // reset draw count to 0
        vkCmdFillBuffer(command_buffer, app->indirect_command_buffer.vk_buffer, offsetof(sb_indirect_command_array, count), sizeof(uint32_t), 0);

        VkBufferMemoryBarrier2 postfill_barrier = sb_get_buffer_barrier(&app->indirect_command_buffer);
        postfill_barrier.srcStageMask = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
        postfill_barrier.dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        postfill_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        postfill_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT |  VK_ACCESS_SHADER_WRITE_BIT;
        sb_buffer_barriers(command_buffer, &postfill_barrier, 1);

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, app->compute_cull_pipeline);
        vkCmdDispatch(command_buffer, SB_MAX_DRAW_COUNT / 64U, 1, 1);

        VkBufferMemoryBarrier2 indirect_barrier = sb_get_buffer_barrier(&app->indirect_command_buffer);
        indirect_barrier.srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        indirect_barrier.dstStageMask = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        indirect_barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT |  VK_ACCESS_SHADER_WRITE_BIT;
        indirect_barrier.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        sb_buffer_barriers(command_buffer, &indirect_barrier, 1);

        sb_texture swapchain_texture = {0};
        swapchain_texture.image = image;
        swapchain_texture.view = image_view;
        swapchain_texture.extent = sb_get_window_extent(app->window);
        swapchain_texture.image_usage = SB_SWAPCHAIN_IMAGE_USAGE;

        swapchain_texture.texture_type = SB_TEXTURE_TYPE_COLOR;
        swapchain_texture.format = app->swapchain_image_format;
        swapchain_texture.memory = VK_NULL_HANDLE; // memory handled by swapchain
        swapchain_texture.sampler = VK_NULL_HANDLE;

        app->bake_command_buffer(app, command_buffer, &swapchain_texture);


        sb_end_command_buffer(command_buffer);
    }
}

void sb_recreate_window_relative_textures(sb_app *app)
{
    VkExtent2D window_extent = sb_get_window_extent(app->window);
    for(sb_texture_id id = 0; id < app->texture_count; id++)
    {
        sb_texture *texture = sb_get_texture(app, id);
        if(texture->is_window_relative)
        {
            vkDestroyImageView(app->device, texture->view, NULL);
            vkDestroyImage(app->device, texture->image, NULL);

            bool was_dedicated_allocation = texture->memory != VK_NULL_HANDLE;
            if(was_dedicated_allocation)
                vkFreeMemory(app->device, texture->memory, NULL);

            texture->image = sb_create_image(app->device, window_extent, texture->format, texture->image_usage, VK_SAMPLE_COUNT_1_BIT);
            if(was_dedicated_allocation)
                texture->memory = sb_dedicated_image_allocation(&app->memory_types, app->device, texture->image);

            texture->extent = window_extent;
            texture->view = sb_create_image_view(app->device, texture->image, texture->format, sb_get_aspect_flag(texture->texture_type));

            if(texture->sampler)
                app->texture_descriptor_updates[app->texture_descriptor_update_count++] = id;
        }
    }
}
void sb_on_resize(sb_app *app)
{
    VK_CHECK(vkDeviceWaitIdle(app->device));

    sb_recreate_window_relative_textures(app);
    sb_recreate_swapchain(app);
    sb_recreate_command_buffers(app);
}

void sb_update_texture_descriptors(sb_app *app)
{
    if(app->texture_descriptor_update_count == 0) return;
	sb_arena_temp scratch = sb_get_scratch();

	VkDescriptorImageInfo *image_infos = sb_arena_push(scratch.arena, VkDescriptorImageInfo, app->texture_descriptor_update_count);
	VkWriteDescriptorSet *set_writes = sb_arena_push(scratch.arena, VkWriteDescriptorSet, app->texture_descriptor_update_count);
	for(int i = 0; i < app->texture_descriptor_update_count; i++)
	{
		sb_texture_id id = app->texture_descriptor_updates[i];
		sb_texture *texture = sb_get_texture(app, id);

		VkDescriptorImageInfo *image_info = &image_infos[i];
		image_info->imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
		image_info->imageView = texture->view;
		image_info->sampler = texture->sampler;

		VkWriteDescriptorSet *write = &set_writes[i];
		write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write->dstSet = app->global_set;
		write->dstBinding = TEXTURE_ARRAY_BINDING;
		write->dstArrayElement = id;
		write->descriptorCount = 1;
		write->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write->pImageInfo = image_info;
	}

	vkUpdateDescriptorSets(app->device, app->texture_descriptor_update_count, set_writes, 0, NULL);

    app->texture_descriptor_update_count = 0;
	sb_release_scratch(&scratch);
}

sb_buffer *sb_get_frame_draw_info_buffer(sb_app *app)
{
    return &app->draw_info_buffers[app->frame_index];
}

void sb_frame(sb_app *app)
{
    sb_wait_for_fence(app->device, app->render_finished_fence);
    sb_reset_fence(app->device, app->render_finished_fence);

    int current_image = sb_acquire_next_image(app->device, app->swapchain, app->image_available_semaphore);
    if(current_image == -1)
    {
        sb_on_resize(app);
        sb_frame(app);
        return;
    }

    sb_update_texture_descriptors(app);
    sb_update_draw_info_buffer_descriptor(app->device, app->global_set, sb_get_frame_draw_info_buffer(app));

    sb_transfer_assets(app->device, &app->transfer_buffer, &app->mesh_memory, app->texture_handles);

    sb_queue_submit_info submit_info = {0};
    submit_info.wait_semaphore = app->image_available_semaphore;
    submit_info.wait_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit_info.signal_semaphore = app->render_finished_semaphore;
    submit_info.signal_stage_mask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    submit_info.fence = app->render_finished_fence;
    submit_info.command_buffer = app->command_buffers[current_image];
	sb_queue_submit(app->graphics_queue, &submit_info);

    if(!sb_present_image(app->swapchain, app->graphics_queue, app->render_finished_semaphore, current_image))
        sb_on_resize(app);

    app->frame_index = ~app->frame_index & 0b1;

    sb_buffer *draw_info_buffer = sb_get_frame_draw_info_buffer(app);
    sb_draw_info_array *info_array = draw_info_buffer->memory_ptr;
    info_array->count = 0;
}

bool sb_run_app(sb_app *app, sb_window_event *window_event)
{
    bool should_quit = sb_poll_events(app->window, window_event);
    if(window_event->flags & SB_WINDOW_RESIZED_FLAG)
    {
        while(app->window->width == 0 || app->window->height == 0)
            sb_wait_events();

        sb_on_resize(app);
    }

    sb_frame(app);
    return should_quit;
}

sb_mesh_id sb_create_mesh(sb_app *app, const char *name)
{
    sb_queue_mesh_transfer(&app->transfer_buffer, name);
	return app->mesh_memory.mesh_count++;
}

void sb_draw(sb_app *app, const sb_draw_info *draw_info)
{
    sb_buffer *draw_info_buffer = sb_get_frame_draw_info_buffer(app);

    sb_draw_info_array *draw_infos = draw_info_buffer->memory_ptr;
    draw_infos->array[draw_infos->count++] = *draw_info;
}
