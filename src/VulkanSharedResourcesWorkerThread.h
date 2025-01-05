#ifndef VRSD_VulkanSharedResourcesWorkerThread_h_
#define VRSD_VulkanSharedResourcesWorkerThread_h_

#include <vulkan/vulkan_core.h>

VK_DEFINE_HANDLE(VmaAllocation);

struct VulkanSharedResourcesWorkerThreadData;
struct VulkanMeshResource;
struct VulkanRenderer;

namespace ftl { class TaskScheduler; }

struct VulkanResourceTransferBufferParams
{
    VulkanRenderer* vulkan_renderer = nullptr;
    VkBuffer buffer = nullptr;
    VmaAllocation allocation;
    void* data = nullptr;
    size_t data_size = 0;
    VulkanMeshResource* vulkan_mesh_resource = nullptr;
};

void vulkan_shared_resources_init_worker_thread_task(ftl::TaskScheduler* task_scheduler, void* arg);

void vulkan_shared_resources_destroy_worker_thread();

void vulkan_shared_resources_add_transfer_buffer_params(VulkanResourceTransferBufferParams* transfer_buffer_params);

#endif // VRSD_VulkanSharedResourcesWorkerThread_h_
