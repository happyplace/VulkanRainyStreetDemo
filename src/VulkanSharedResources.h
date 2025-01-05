#ifndef VRSD_VulkanSharedResources_h_
#define VRSD_VulkanSharedResources_h_

#include <atomic>
#include <cstdint>

#include <vulkan/vulkan_core.h>

#include "VulkanSharedResourcesWorkerThread.h"

VK_DEFINE_HANDLE(VmaAllocation);

namespace ftl { class TaskScheduler; }

struct VulkanRenderer;

struct VulkanMeshResource
{
    std::atomic<uint8_t> resource_loaded_count = 0;
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

struct VulkanSharedResourcesInitParams
{
    VulkanRenderer* vulkan_renderer = nullptr;
    VulkanSharedResources* vulkan_shared_resources = nullptr;
    bool init_successful = false;
};

void vulkan_shared_resources_init_task(ftl::TaskScheduler* task_scheduler, void* arg);
void vulkan_shared_resources_destroy(VulkanSharedResources* vulkan_shared_resources, struct VulkanRenderer* vulkan_renderer);

VkSampler vulkan_shared_resources_get_sampler(VulkanSharedResources* vulkan_shared_resources, VulkanSharedResourcesSamplerType sampler_type);
uint32_t vulkan_shared_resources_get_frame_buffer_offset(VulkanSharedResources* vulkan_shared_resources, struct FrameResource* frame_resource);
VulkanMeshResource* vulkan_shared_resources_get_mesh(VulkanSharedResources* vulkan_shared_resources, VulkanMeshType vulkan_mesh_type);

#endif // VRSD_VulkanSharedResources_h_
