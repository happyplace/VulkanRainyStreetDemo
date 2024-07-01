#include "VulkanFrameResources.h"

#include <SDL_assert.h>
#include <vulkan/vulkan_core.h>

#include "VulkanRenderer.h"

struct VulkanFrameResources
{
    FrameResource frame_resources[VULKAN_FRAME_RESOURCES_FRAME_RESOURCE_COUNT];
    uint32_t next_frame_resource_index = 0;
};

void vulkan_frame_resources_clear_frame_resources(VulkanFrameResources* vulkan_frame_resources)
{
    for (uint32_t i = 0; i < VULKAN_FRAME_RESOURCES_FRAME_RESOURCE_COUNT; ++i)
    {
        FrameResource* frame_resource = &vulkan_frame_resources->frame_resources[i];

        frame_resource->acquire_image = VK_NULL_HANDLE;
        frame_resource->release_image = VK_NULL_HANDLE;
        frame_resource->submit_fence = VK_NULL_HANDLE;
        frame_resource->command_pool = VK_NULL_HANDLE;
        frame_resource->command_buffer = VK_NULL_HANDLE;
    }
}

VulkanFrameResources* vulkan_frame_resources_init(struct VulkanRenderer* vulkan_renderer)
{
    VulkanFrameResources* vulkan_frame_resources = new VulkanFrameResources();

    vulkan_frame_resources_clear_frame_resources(vulkan_frame_resources);

    for (uint32_t i = 0; i < VULKAN_FRAME_RESOURCES_FRAME_RESOURCE_COUNT; ++i)
    {
        FrameResource* frame_resource = &vulkan_frame_resources->frame_resources[i];

        VkSemaphoreCreateInfo semaphore_create_info;
        semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphore_create_info.pNext = nullptr;
        semaphore_create_info.flags = 0;
        VkResult result = vkCreateSemaphore(vulkan_renderer->device, &semaphore_create_info, nullptr, &frame_resource->acquire_image);
        if (result != VK_SUCCESS)
        {
            return nullptr;
        }
        result = vkCreateSemaphore(vulkan_renderer->device, &semaphore_create_info, nullptr, &frame_resource->release_image);
        if (result != VK_SUCCESS)
        {
            return nullptr;
        }

        VkFenceCreateInfo fence_create_info;
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_create_info.pNext = nullptr;
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        result = vkCreateFence(vulkan_renderer->device, &fence_create_info, nullptr, &frame_resource->submit_fence);
        if (result != VK_SUCCESS)
        {
            return nullptr;
        }

        VkCommandPoolCreateInfo command_pool_create_info;
        command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        command_pool_create_info.pNext = nullptr;
        command_pool_create_info.flags = 0;
        command_pool_create_info.queueFamilyIndex = vulkan_renderer->graphics_queue_index;
        result = vkCreateCommandPool(vulkan_renderer->device, &command_pool_create_info, nullptr, &frame_resource->command_pool);
        if (result != VK_SUCCESS)
        {
            return nullptr;
        }

        VkCommandBufferAllocateInfo command_buffer_allocate_info;
        command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        command_buffer_allocate_info.pNext = nullptr;
        command_buffer_allocate_info.commandPool = frame_resource->command_pool;
        command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        command_buffer_allocate_info.commandBufferCount = 1;
        result = vkAllocateCommandBuffers(vulkan_renderer->device, &command_buffer_allocate_info, &frame_resource->command_buffer);
        if (result != VK_SUCCESS)
        {
            return nullptr;
        }
    }

    return vulkan_frame_resources;
}

void vulkan_frame_resources_destroy(struct VulkanRenderer* vulkan_renderer, VulkanFrameResources* vulkan_frame_resources)
{
    SDL_assert(vulkan_renderer);
    SDL_assert(vulkan_frame_resources);

    vkDeviceWaitIdle(vulkan_renderer->device);

    for (uint32_t i = 0; i < VULKAN_FRAME_RESOURCES_FRAME_RESOURCE_COUNT; ++i)
    {
        FrameResource* frame_resource = &vulkan_frame_resources->frame_resources[i];

        if (frame_resource->command_buffer)
        {
            vkFreeCommandBuffers(vulkan_renderer->device, frame_resource->command_pool, 1, &frame_resource->command_buffer);
        }

        if (frame_resource->command_pool)
        {
            vkDestroyCommandPool(vulkan_renderer->device, frame_resource->command_pool, nullptr);
        }

        if (frame_resource->submit_fence)
        {
            vkDestroyFence(vulkan_renderer->device, frame_resource->submit_fence, nullptr);
        }

        if (frame_resource->acquire_image)
        {
            vkDestroySemaphore(vulkan_renderer->device, frame_resource->acquire_image, nullptr);
        }

        if (frame_resource->release_image)
        {
            vkDestroySemaphore(vulkan_renderer->device, frame_resource->release_image, nullptr);
        }
    }

    delete vulkan_frame_resources;
}

FrameResource* vulkan_frame_resources_get_next_frame_resource(VulkanFrameResources* vulkan_frame_resources)
{
    FrameResource* frame_resource = &vulkan_frame_resources->frame_resources[vulkan_frame_resources->next_frame_resource_index];

    vulkan_frame_resources->next_frame_resource_index++;
    if (vulkan_frame_resources->next_frame_resource_index >= VULKAN_FRAME_RESOURCES_FRAME_RESOURCE_COUNT)
    {
        vulkan_frame_resources->next_frame_resource_index = 0;
    }

    return frame_resource;
}
