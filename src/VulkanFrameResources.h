#ifndef VRSD_VulkanFrameResources_h_
#define VRSD_VulkanFrameResources_h_

#include <cstdint>

#include <vulkan/vulkan_core.h>

constexpr uint32_t VULKAN_FRAME_RESOURCES_FRAME_RESOURCE_COUNT = 2;

struct FrameResourceTime
{
    double delta_time;
    double total_time;
    uint64_t frame_number;
};

struct FrameResource
{
    VkSemaphore acquire_image = VK_NULL_HANDLE;
    VkSemaphore release_image = VK_NULL_HANDLE;
    VkFence submit_fence = VK_NULL_HANDLE;
    VkCommandPool command_pool = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    uint32_t swapchain_image_index = 0;
    FrameResourceTime time;
};

struct VulkanFrameResources;

VulkanFrameResources* vulkan_frame_resources_init(struct VulkanRenderer* vulkan_renderer);
void vulkan_frame_resources_destroy(VulkanFrameResources* vulkan_frame_resources, struct VulkanRenderer* vulkan_renderer);

void vulkan_frame_resources_pop_next_frame_resource(VulkanFrameResources* vulkan_frame_resources);
FrameResource* vulkan_frame_resources_peek_next_frame_resource(VulkanFrameResources* vulkan_frame_resources);

#endif // VRSD_VulkanFrameResources_h_
