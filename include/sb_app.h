#ifndef SB_APP_H
#define SB_APP_H

#include "sb_window.h"
#include "sb_transfer_buffer.h"
#include "sb_texture.h"
#include "sb_mesh.h"

#define SB_MAX_DRAW_COUNT 65535 //2^16 = 1, min limit by vulkan sppec

typedef struct
{
	sb_mat4 transform;

	uint32_t mesh_id;
	uint32_t texture_id;
    float specularity;
    uint32_t shader_id;

    uint32_t color;
    uint32_t pad[3];
} sb_draw_info;

typedef struct
{
	uint32_t count;
    uint32_t pad[3];
	sb_draw_info array[SB_MAX_DRAW_COUNT];
} sb_draw_info_array;

typedef struct
{
	uint32_t count;
	VkDrawIndexedIndirectCommand array[SB_MAX_DRAW_COUNT];
} sb_indirect_command_array;

typedef struct
{
    float constant_factor;
    float slope_factor;
} sb_depth_bias;

typedef struct
{
	uint32_t color_attachment_count;
	const VkFormat *color_attachment_formats;

	VkFormat depth_attachment_format;
	const sb_depth_bias *depth_bias;
    bool cull_enabled;

    const char *vertex_shader_name;
    const VkDeviceAddress *vertex_ubos;
    uint32_t vertex_ubo_count;

    const char *fragment_shader_name;
    const VkDeviceAddress *fragment_ubos;
    uint32_t fragment_ubos_count;
} sb_graphics_pipeline_info;

typedef struct
{
    const char *compute_shader_name;
    const VkDeviceAddress *addresses;
    uint32_t address_count;
} sb_compute_pipeline_info;

typedef struct sb_app
{
    const void *user_data;
    const void (*bake_command_buffer) (struct sb_app *app, VkCommandBuffer command_buffer, sb_texture *swapchain_texture);

    sb_window *window;

    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical_device;
    VkDevice device;

    VkFormat swapchain_image_format;
    VkPresentModeKHR present_mode;
    VkSwapchainKHR swapchain;

    uint32_t image_count;
    sb_arena *swapchain_arena;
    VkImage *images;
    VkImageView *image_views;
    VkCommandBuffer *command_buffers;
    VkCommandPool command_pool;

    sb_memory_types memory_types;

    VkDescriptorPool descriptor_pool;
    VkDescriptorSetLayout global_set_layout;
    VkDescriptorSet global_set;

    VkQueue graphics_queue;
    VkSemaphore image_available_semaphore;
    VkSemaphore render_finished_semaphore;
    VkFence render_finished_fence;

    VkPipelineLayout global_pipeline_layout;
    VkPipeline compute_cull_pipeline;

    sb_mesh_memory mesh_memory;

    sb_texture texture_handles[SB_MAX_TEXTURES];
	uint32_t texture_count;

	sb_texture_id texture_descriptor_updates[SB_MAX_TEXTURES];
	uint32_t texture_descriptor_update_count;

    sb_transfer_buffer transfer_buffer;

	sb_device_arena image_arena;

    sb_device_arena ubo_staging_arena;
    sb_buffer ubo_gpu_memory;

    sb_buffer indirect_command_buffer;
    sb_buffer draw_info_buffers[2];

    uint8_t frame_index;
} sb_app;

static VkSpecializationInfo *get_specialization_info(sb_arena *arena, const VkDeviceAddress *addresses, uint32_t address_count);
VkPipeline sb_create_graphics_pipeline(sb_app *app, const sb_graphics_pipeline_info *pipeline_info);
VkPipeline sb_create_compute_pipeline(sb_app *app, const sb_compute_pipeline_info *pipeline_info);

void *sb_alloc_ubo(sb_app *app, VkDeviceSize size, VkDeviceAddress *out_address);

typedef struct
{
    VkFormat format;
    VkExtent2D extent;
    VkSamplerAddressMode sampler_address_mode;
    VkBorderColor sampler_border_color;
    sb_texture_type texture_type;

    uint32_t usage;
    uint32_t flags;
} sb_texture_info;

sb_texture_id sb_create_texture(sb_app *app, const sb_texture_info *info);
sb_texture_id sb_texture_from_file(sb_app *app, const char *file_path);
sb_texture *sb_get_texture(sb_app *app, sb_texture_id id);

typedef enum
{
    SB_IMAGE_LAYOUT_UNDEFINED = VK_IMAGE_LAYOUT_UNDEFINED,
    SB_IMAGE_LAYOUT_RENDER_ATTACHMENT = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
    SB_IMAGE_LAYOUT_READ_ONLY = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
    SB_IMAGE_LAYOUT_PRESENT = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
} sb_image_layout;

typedef struct
{
    const sb_texture *texture;

    sb_image_layout old_layout;
    sb_image_layout new_layout;

    VkPipelineStageFlags2 src_stage_mask;
    VkPipelineStageFlags2 dst_stage_mask;
} sb_image_transition;

static VkAccessFlags2 get_access_mask(VkImageLayout layout, sb_texture_type texture_type);
void sb_set_image_layouts(VkCommandBuffer command_buffer, const sb_image_transition *texture_barriers, uint32_t count);

typedef union
{
    sb_color3 color;
    float depth;
} sb_clear_value;

VkRenderingAttachmentInfo sb_rendering_attachment_info(sb_texture *texture, sb_clear_value clear_value);

typedef struct
{
    const VkRenderingAttachmentInfo *color_attachments;
    uint32_t color_attachment_count;

    const VkRenderingAttachmentInfo *depth_attachment;
    VkExtent2D render_area;
} sb_render_pass_info;

void sb_begin_render_pass(VkCommandBuffer command_buffer, sb_render_pass_info *info);
#define sb_bind_graphics_pipeline(command_buffer, pipeline) vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline)
void sb_draw_scene(VkCommandBuffer command_buffer, sb_buffer *draw_command_buffer);
void sb_draw_pixels(VkCommandBuffer command_buffer);
#define sb_end_render_pass(command_buffer) vkCmdEndRendering(command_buffer)

typedef struct
{
    const char *name;
    void (*bake_command_buffer) (sb_app *app, VkCommandBuffer command_buffer, sb_texture *swapchain_texture);
    int window_height;
    int window_width;
} sb_app_info;

sb_app *sb_create_app(const sb_app_info *app_info);

static void sb_update_texture_descriptors(sb_app *app);
static void sb_recreate_window_relative_textures(sb_app *app);
static void sb_recreate_swapchain(sb_app *app);
static void sb_recreate_command_buffers(sb_app *app);
static sb_buffer *sb_get_frame_draw_info_buffer(sb_app *app);

void sb_on_resize(sb_app *app);
void sb_frame(sb_app *app);
bool sb_run_app(sb_app *app, sb_window_event *window_event);

sb_mesh_id sb_create_mesh(sb_app *app, const char *name);
void sb_draw(sb_app *app, const sb_draw_info *draw_info);

#endif
