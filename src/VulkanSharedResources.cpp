#include "VulkanSharedResources.h"

#include <SDL_assert.h>

#include <vulkan/vulkan.h>

#include "VulkanRenderer.h"

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

void vulkan_shared_resources_destroy(VulkanSharedResources* vulkan_shared_resources, struct VulkanRenderer* vulkan_renderer)
{
    SDL_assert(vulkan_shared_resources);
    SDL_assert(vulkan_renderer);

    vulkan_renderer_wait_device_idle(vulkan_renderer);

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

VulkanSharedResources* vulkan_shared_resources_init(struct VulkanRenderer* vulkan_renderer)
{
    VulkanSharedResources* vulkan_shared_resources = new VulkanSharedResources();

    if (!vulkan_shared_resources_init_samplers(vulkan_shared_resources, vulkan_renderer))
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
