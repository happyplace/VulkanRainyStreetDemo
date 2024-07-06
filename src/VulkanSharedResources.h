#ifndef VRSD_VulkanSharedResources_h_
#define VRSD_VulkanSharedResources_h_

#include <cstdint>

#include <vulkan/vulkan_core.h>

enum class VulkanSharedResourcesSamplerType : uint8_t
{
    Linear,
    COUNT,
};

struct VulkanSharedResources
{
    VkSampler* samplers = nullptr;
};

VulkanSharedResources* vulkan_shared_resources_init(struct VulkanRenderer* vulkan_renderer);
void vulkan_shared_resources_destroy(VulkanSharedResources* vulkan_shared_resources, struct VulkanRenderer* vulkan_renderer);

VkSampler vulkan_shared_resources_get_sampler(VulkanSharedResources* vulkan_shared_resources, VulkanSharedResourcesSamplerType sampler_type);

#endif // VRSD_VulkanSharedResources_h_
