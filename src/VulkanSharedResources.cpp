#include "VulkanSharedResources.h"

#include <SDL_assert.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "vk_mem_alloc.h"

#include "VulkanRenderer.h"
#include "VulkanFrameResources.h"

void vulkan_renderer_destroy_samplers(VulkanSharedResources* vulkan_shared_resources, VulkanRenderer* vulkan_renderer)
{
    if (vulkan_shared_resources->samplers != nullptr)
    {
        for (size_t i = 0; i < static_cast<size_t>(VulkanSharedResourcesSamplerType::COUNT); ++i)
        {
            if (vulkan_shared_resources->samplers[i] != VK_NULL_HANDLE)
            {
                vkDestroySampler(vulkan_renderer->device, vulkan_shared_resources->samplers[i], s_allocator);
            }
        }

        delete[] vulkan_shared_resources->samplers;
        vulkan_shared_resources->samplers = nullptr;
    }
}

void vulkan_shared_resources_destroy_frame_buffer(VulkanSharedResources* vulkan_shared_resources, VulkanRenderer* vulkan_renderer)
{
    if (vulkan_shared_resources->frame_allocation != VK_NULL_HANDLE || vulkan_shared_resources->frame_buffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(vulkan_renderer->vma_allocator, vulkan_shared_resources->frame_buffer, vulkan_shared_resources->frame_allocation);

        vulkan_shared_resources->frame_allocation = VK_NULL_HANDLE;
        vulkan_shared_resources->frame_buffer = VK_NULL_HANDLE;
    }
}

void vulkan_shared_resources_destroy(VulkanSharedResources* vulkan_shared_resources, struct VulkanRenderer* vulkan_renderer)
{
    SDL_assert(vulkan_shared_resources);
    SDL_assert(vulkan_renderer);

    vulkan_renderer_wait_device_idle(vulkan_renderer);

    vulkan_shared_resources_destroy_frame_buffer(vulkan_shared_resources, vulkan_renderer);
    vulkan_renderer_destroy_samplers(vulkan_shared_resources, vulkan_renderer);
}

bool vulkan_shared_resources_init_samplers(VulkanSharedResources* vulkan_shared_resources, VulkanRenderer* vulkan_renderer)
{
    vulkan_shared_resources->samplers = new VkSampler[static_cast<size_t>(VulkanSharedResourcesSamplerType::COUNT)];
    for (size_t i = 0; i < static_cast<size_t>(VulkanSharedResourcesSamplerType::COUNT); ++i)
    {
        vulkan_shared_resources->samplers[i] = VK_NULL_HANDLE;
    }

    VkPhysicalDeviceFeatures physical_device_features;
    vkGetPhysicalDeviceFeatures(vulkan_renderer->physical_device, &physical_device_features);

    VkPhysicalDeviceProperties physical_device_properties;
    vkGetPhysicalDeviceProperties(vulkan_renderer->physical_device, &physical_device_properties);

    {
        VkSamplerCreateInfo sampler_create_info;
        sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_create_info.pNext = nullptr;
        sampler_create_info.flags = 0;
        sampler_create_info.magFilter = VK_FILTER_LINEAR;
        sampler_create_info.minFilter = VK_FILTER_LINEAR;
        sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_create_info.mipLodBias = 0.0f;
        sampler_create_info.anisotropyEnable = physical_device_features.samplerAnisotropy ? VK_TRUE : VK_FALSE;
        sampler_create_info.maxAnisotropy = physical_device_features.samplerAnisotropy ? physical_device_properties.limits.maxSamplerAnisotropy : 1.0f;
        sampler_create_info.compareEnable = VK_FALSE;
        sampler_create_info.compareOp = VK_COMPARE_OP_NEVER;
        sampler_create_info.minLod = 0.0f;
        sampler_create_info.maxLod = 0.0f;
        sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        sampler_create_info.unnormalizedCoordinates = VK_FALSE;

        VkResult result = vkCreateSampler(vulkan_renderer->device, &sampler_create_info, s_allocator,
            &vulkan_shared_resources->samplers[static_cast<size_t>(VulkanSharedResourcesSamplerType::Linear)]);
        if (result != VK_SUCCESS)
        {
            return false;
        }
    }

    return true;
}

bool vulkan_shared_resources_init_frame_buffer(VulkanSharedResources* vulkan_shared_resources, VulkanRenderer* vulkan_renderer)
{
    vulkan_shared_resources->frame_alignment_size = vulkan_renderer_calculate_uniform_buffer_size(vulkan_renderer, sizeof(Vulkan_FrameBuffer));

    VkBufferCreateInfo buffer_create_info;
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.pNext = nullptr;
    buffer_create_info.flags = 0;
    buffer_create_info.size = vulkan_shared_resources->frame_alignment_size * VULKAN_FRAME_RESOURCES_FRAME_RESOURCE_COUNT;
    buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.queueFamilyIndexCount = 0;
    buffer_create_info.pQueueFamilyIndices = nullptr;

    VmaAllocationCreateInfo allocation_create_info;
    allocation_create_info.flags = 0;
    allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
    allocation_create_info.requiredFlags = 0;
    allocation_create_info.preferredFlags = 0;
    allocation_create_info.memoryTypeBits = 0;
    allocation_create_info.pool = VK_NULL_HANDLE;
    allocation_create_info.pUserData = nullptr;
    allocation_create_info.priority = 0.0f;

    VkResult result = vmaCreateBuffer(vulkan_renderer->vma_allocator, &buffer_create_info, &allocation_create_info,
        &vulkan_shared_resources->frame_buffer, &vulkan_shared_resources->frame_allocation, nullptr);

    return result == VK_SUCCESS;
}

VulkanSharedResources* vulkan_shared_resources_init(struct VulkanRenderer* vulkan_renderer)
{
    VulkanSharedResources* vulkan_shared_resources = new VulkanSharedResources();

    if (!vulkan_shared_resources_init_samplers(vulkan_shared_resources, vulkan_renderer))
    {
        vulkan_shared_resources_destroy(vulkan_shared_resources, vulkan_renderer);
        return nullptr;
    }

    if (!vulkan_shared_resources_init_frame_buffer(vulkan_shared_resources, vulkan_renderer))
    {
        vulkan_shared_resources_destroy(vulkan_shared_resources, vulkan_renderer);
        return nullptr;
    }

    return vulkan_shared_resources;
}

VkSampler vulkan_shared_resources_get_sampler(VulkanSharedResources* vulkan_shared_resources, VulkanSharedResourcesSamplerType sampler_type)
{
    uint32_t sampler_type_index = static_cast<uint32_t>(sampler_type);
    if (sampler_type_index >= static_cast<uint32_t>(VulkanSharedResourcesSamplerType::COUNT))
    {
        SDL_assert(false);
        return VK_NULL_HANDLE;
    }

    return vulkan_shared_resources->samplers[sampler_type_index];
}

uint32_t vulkan_shared_resources_get_frame_buffer_offset(VulkanSharedResources* vulkan_shared_resources, struct FrameResource* frame_resource)
{
    return static_cast<uint32_t>(vulkan_shared_resources->frame_alignment_size * frame_resource->index);
}
