#ifndef VRSD_VulkanSharedResources_h_
#define VRSD_VulkanSharedResources_h_

#include <cstdint>

#include <vulkan/vulkan_core.h>

#include <DirectXMath.h>

#include "VrsdStd.h"
#include "RenderDefines.h"

VK_DEFINE_HANDLE(VmaAllocation);

struct VRSD_ALIGN(16) Vulkan_FrameBuffer
{
    DirectX::XMFLOAT4X4 view_projection;
    DirectX::XMFLOAT3 eye_position;
};

struct VulkanMeshResource
{
    VkBuffer vertex_buffer = VK_NULL_HANDLE;
    VmaAllocation vertex_allocation = VK_NULL_HANDLE;
    VkBuffer index_buffer = VK_NULL_HANDLE;
    VmaAllocation index_allocation = VK_NULL_HANDLE;
    uint32_t index_count = 0;
};

enum class VulkanMeshType : uint8_t
{
    Cube,
    COUNT,
};

enum class VulkanSharedResourcesSamplerType : uint8_t
{
    Linear,
    COUNT,
};

struct VulkanSharedResources
{
    VkSampler* samplers = nullptr;
    VkBuffer frame_buffer = VK_NULL_HANDLE;
    VmaAllocation frame_allocation = VK_NULL_HANDLE;
    VkDeviceSize frame_alignment_size = 0;
    VulkanMeshResource* mesh_resources = nullptr;
};

VulkanSharedResources* vulkan_shared_resources_init(struct VulkanRenderer* vulkan_renderer);
void vulkan_shared_resources_destroy(VulkanSharedResources* vulkan_shared_resources, struct VulkanRenderer* vulkan_renderer);

VkSampler vulkan_shared_resources_get_sampler(VulkanSharedResources* vulkan_shared_resources, VulkanSharedResourcesSamplerType sampler_type);
uint32_t vulkan_shared_resources_get_frame_buffer_offset(VulkanSharedResources* vulkan_shared_resources, struct FrameResource* frame_resource);

VulkanMeshResource* vulkan_shared_resources_get_mesh(VulkanSharedResources* vulkan_shared_resources, VulkanMeshType vulkan_mesh_type);

#endif // VRSD_VulkanSharedResources_h_
