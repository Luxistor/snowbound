#ifndef SB_TEXTURE_H
#define SB_TEXTURE_H

#include <vulkan/vulkan.h>

#define SB_MAX_TEXTURES 1024U
#define SB_NULL_TEXTURE_ID 0U

typedef enum
{
    SB_TEXTURE_TYPE_DEPTH,
    SB_TEXTURE_TYPE_COLOR,
} sb_texture_type;
\
typedef enum
{
	SB_TEXTURE_USAGE_SHADER_READ_FLAG = (1<<1),
	SB_TEXTURE_USAGE_RENDER_ATTACHMENT_FLAG = (1<<2),
	SB_TEXTURE_USAGE_TRANSFER_DST = (1<<3),
} sb_texture_usage;

typedef enum
{
    SB_TEXTURE_FLAG_DEDICATED_ALLOCATION = (1<<1),
    SB_TEXTURE_FLAG_WINDOW_RELATIVE = (1<<2),
} sb_texture_flags;

typedef uint32_t sb_texture_id;

typedef struct
{
	sb_texture_type texture_type;

	VkImageUsageFlags image_usage;
	VkFormat format;
	VkSampler sampler;
	VkImage image;
	VkExtent2D extent;
	VkImageView view;
	VkDeviceMemory memory;
	VkDeviceSize offset;

	bool is_window_relative;
} sb_texture;

#endif