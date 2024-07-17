#include "sb_mesh.h"

sb_mesh_memory sb_alloc_mesh_memory(VkDevice device, const sb_memory_types *memory_types)
{
    sb_mesh_memory meshes = {0};

    sb_memory_info index_buffer_info = {0};
    index_buffer_info.capacity = MB(64);
    index_buffer_info.memory_usage = SB_MEMORY_USAGE_GPU;
    index_buffer_info.buffer_usage_flags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    index_buffer_info.memory_types = memory_types;
    sb_allocate_buffer(device, &index_buffer_info, &meshes.index_buffer);

    sb_memory_info vertex_buffer_info = {0};
    vertex_buffer_info.capacity = MB(64);
    vertex_buffer_info.memory_usage = SB_MEMORY_USAGE_GPU;
    vertex_buffer_info.buffer_usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    vertex_buffer_info.memory_types = memory_types;
    sb_allocate_buffer(device, &vertex_buffer_info, &meshes.vertex_buffer);

    sb_memory_info handle_buffer_info = {0};
    handle_buffer_info.capacity = sizeof(sb_mesh_handle)*SB_MAX_MESHES;
    handle_buffer_info.memory_usage = SB_MEMORY_USAGE_GPU;
    handle_buffer_info.buffer_usage_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    handle_buffer_info.memory_types = memory_types;
    sb_allocate_buffer(device, &handle_buffer_info, &meshes.handle_buffer);
    return meshes;
}

void sb_bind_mesh_buffers(VkCommandBuffer command_buffer, sb_mesh_memory *meshes)
{
    const VkDeviceSize vertexOffset = 0;
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &meshes->vertex_buffer.vk_buffer, &vertexOffset);
	vkCmdBindIndexBuffer(command_buffer, meshes->index_buffer.vk_buffer, 0, VK_INDEX_TYPE_UINT32);
}