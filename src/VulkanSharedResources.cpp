#include "VulkanSharedResources.h"

#include <SDL_assert.h>

#include <cstring>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "vk_mem_alloc.h"

#include "VulkanRenderer.h"
#include "VulkanFrameResources.h"
#include "GeometryLoader.h"

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

void vulkan_shared_resources_destroy_mesh_resources(VulkanSharedResources* vulkan_shared_resources, VulkanRenderer* vulkan_renderer)
{
    if (vulkan_shared_resources->mesh_resources != nullptr)
    {
        for (size_t i = 0; i < static_cast<size_t>(VulkanMeshType::COUNT); ++i)
        {
            VulkanMeshResource& vulkan_mesh_resource = vulkan_shared_resources->mesh_resources[i];
            if (vulkan_mesh_resource.vertex_buffer != VK_NULL_HANDLE || vulkan_mesh_resource.vertex_allocation != VK_NULL_HANDLE)
            {
                vmaDestroyBuffer(vulkan_renderer->vma_allocator, vulkan_mesh_resource.vertex_buffer, vulkan_mesh_resource.vertex_allocation);
            }

            if (vulkan_mesh_resource.index_buffer != VK_NULL_HANDLE || vulkan_mesh_resource.index_allocation != VK_NULL_HANDLE)
            {
                vmaDestroyBuffer(vulkan_renderer->vma_allocator, vulkan_mesh_resource.index_buffer, vulkan_mesh_resource.index_allocation);
            }
        }

        delete[] vulkan_shared_resources->mesh_resources;
        vulkan_shared_resources->mesh_resources = nullptr;
    }
}

void vulkan_shared_resources_destroy(VulkanSharedResources* vulkan_shared_resources, struct VulkanRenderer* vulkan_renderer)
{
    SDL_assert(vulkan_shared_resources);
    SDL_assert(vulkan_renderer);

    vulkan_renderer_wait_device_idle(vulkan_renderer);

    vulkan_shared_resources_destroy_mesh_resources(vulkan_shared_resources, vulkan_renderer);
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
    allocation_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
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

bool vulkan_shared_resources_transfer_buffer(VulkanRenderer* vulkan_renderer, VkBuffer buffer, VmaAllocation allocation, void* data, size_t data_size)
{
    VkMemoryPropertyFlags memory_property_flags;
    vmaGetAllocationMemoryProperties(vulkan_renderer->vma_allocator, allocation, &memory_property_flags);

    if (memory_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        void* mapped_data;
        VkResult result = vmaMapMemory(vulkan_renderer->vma_allocator, allocation, &mapped_data);
        if (result != VK_SUCCESS)
        {
            return false;
        }
        memcpy(mapped_data, data, data_size);
        if (!(memory_property_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        {
            vmaFlushAllocation(vulkan_renderer->vma_allocator, allocation, 0, VK_WHOLE_SIZE);
        }
        vmaUnmapMemory(vulkan_renderer->vma_allocator, allocation);
    }
    else
    {
        VkBufferCreateInfo buffer_create_info;
        buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.pNext = nullptr;
        buffer_create_info.flags = 0;
        buffer_create_info.size = static_cast<VkDeviceSize>(data_size);
        buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffer_create_info.queueFamilyIndexCount = 0;
        buffer_create_info.pQueueFamilyIndices = nullptr;

        VmaAllocationCreateInfo allocation_create_info;
        allocation_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        allocation_create_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        allocation_create_info.requiredFlags = 0;
        allocation_create_info.preferredFlags = 0;
        allocation_create_info.memoryTypeBits = 0;
        allocation_create_info.pool = VK_NULL_HANDLE;
        allocation_create_info.pUserData = nullptr;
        allocation_create_info.priority = 0.0f;

        VkBuffer staging_buffer;
        VmaAllocation staging_allocation;
        VmaAllocationInfo staging_allocation_info;
        VkResult result = vmaCreateBuffer(vulkan_renderer->vma_allocator, &buffer_create_info, &allocation_create_info, &staging_buffer, &staging_allocation, &staging_allocation_info);
        if (result != VK_SUCCESS)
        {
            return false;
        }

        memcpy(staging_allocation_info.pMappedData, data, data_size);
        VK_ASSERT(vmaFlushAllocation(vulkan_renderer->vma_allocator, staging_allocation, 0, VK_WHOLE_SIZE));

        VK_ASSERT(vkWaitForFences(vulkan_renderer->device, 1, &vulkan_renderer->transfer_fence, VK_TRUE, UINT64_MAX));

        VK_ASSERT(vkResetFences(vulkan_renderer->device, 1, &vulkan_renderer->transfer_fence));
        VK_ASSERT(vkResetCommandPool(vulkan_renderer->device, vulkan_renderer->transfer_command_pool, 0));

        VkCommandBufferBeginInfo command_buffer_begin_info;
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr;
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        command_buffer_begin_info.pInheritanceInfo = nullptr;
        VK_ASSERT(vkBeginCommandBuffer(vulkan_renderer->transfer_command_buffer, &command_buffer_begin_info));

        VkBufferCopy buffer_copy;
        buffer_copy.srcOffset = 0;
        buffer_copy.dstOffset = 0;
        buffer_copy.size = static_cast<VkDeviceSize>(data_size);
        vkCmdCopyBuffer(vulkan_renderer->transfer_command_buffer, staging_buffer, buffer, 1, &buffer_copy);

        VK_ASSERT(vkEndCommandBuffer(vulkan_renderer->transfer_command_buffer));

        VkSubmitInfo submit_info;
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pWaitDstStageMask = nullptr;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &vulkan_renderer->transfer_command_buffer;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = nullptr;

        VK_ASSERT(vkQueueSubmit(vulkan_renderer->graphics_queue, 1, &submit_info, vulkan_renderer->transfer_fence));

        VK_ASSERT(vkWaitForFences(vulkan_renderer->device, 1, &vulkan_renderer->transfer_fence, VK_TRUE, UINT64_MAX));

        vmaDestroyBuffer(vulkan_renderer->vma_allocator, staging_buffer, staging_allocation);
    }

    return true;
}

bool vulkan_shared_resources_init_mesh_resource_with_geometry(Geometry* geometry, VulkanRenderer* vulkan_renderer, VulkanMeshResource* out_vulkan_mesh_resource)
{
    if (geometry == nullptr)
    {
        SDL_assert(false);
        return false;
    }

    VkDeviceSize vertex_total_size = sizeof(Vertex) * geometry->vertex_count;

    VkBufferCreateInfo vertex_buffer_create_info;
    vertex_buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertex_buffer_create_info.pNext = nullptr;
    vertex_buffer_create_info.flags = 0;
    vertex_buffer_create_info.size = vertex_total_size;
    vertex_buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vertex_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vertex_buffer_create_info.queueFamilyIndexCount = 0;
    vertex_buffer_create_info.pQueueFamilyIndices = nullptr;

    VmaAllocationCreateInfo vertex_allocation_create_info;
    vertex_allocation_create_info.flags = 0;
    vertex_allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
    vertex_allocation_create_info.requiredFlags = 0;
    vertex_allocation_create_info.preferredFlags = 0;
    vertex_allocation_create_info.memoryTypeBits = 0;
    vertex_allocation_create_info.pool = VK_NULL_HANDLE;
    vertex_allocation_create_info.pUserData = nullptr;
    vertex_allocation_create_info.priority = 0.0f;

    VkResult result = vmaCreateBuffer(vulkan_renderer->vma_allocator, &vertex_buffer_create_info, &vertex_allocation_create_info,
        &out_vulkan_mesh_resource->vertex_buffer, &out_vulkan_mesh_resource->vertex_allocation, nullptr);

    if (result != VK_SUCCESS)
    {
        return false;
    }

    if (!vulkan_shared_resources_transfer_buffer(
        vulkan_renderer,
        out_vulkan_mesh_resource->vertex_buffer,
        out_vulkan_mesh_resource->vertex_allocation,
        geometry_get_vertex(geometry),
        static_cast<size_t>(vertex_total_size)))
    {
        return false;
    }

    VkDeviceSize index_total_size = sizeof(uint32_t) * geometry->index_count;

    VkBufferCreateInfo index_buffer_create_info;
    index_buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    index_buffer_create_info.pNext = nullptr;
    index_buffer_create_info.flags = 0;
    index_buffer_create_info.size = index_total_size;
    index_buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    index_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    index_buffer_create_info.queueFamilyIndexCount = 0;
    index_buffer_create_info.pQueueFamilyIndices = nullptr;

    VmaAllocationCreateInfo index_allocation_create_info;
    index_allocation_create_info.flags = 0;
    index_allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
    index_allocation_create_info.requiredFlags = 0;
    index_allocation_create_info.preferredFlags = 0;
    index_allocation_create_info.memoryTypeBits = 0;
    index_allocation_create_info.pool = VK_NULL_HANDLE;
    index_allocation_create_info.pUserData = nullptr;
    index_allocation_create_info.priority = 0.0f;

    result = vmaCreateBuffer(vulkan_renderer->vma_allocator, &index_buffer_create_info, &index_allocation_create_info,
        &out_vulkan_mesh_resource->index_buffer, &out_vulkan_mesh_resource->index_allocation, nullptr);

    if (result != VK_SUCCESS)
    {
        return false;
    }

    if (!vulkan_shared_resources_transfer_buffer(
        vulkan_renderer,
        out_vulkan_mesh_resource->index_buffer,
        out_vulkan_mesh_resource->index_allocation,
        geometry_get_index(geometry),
        static_cast<size_t>(index_total_size)))
    {
        return false;
    }

    return true;
}

bool vulkan_shared_resources_init_mesh_resources(VulkanSharedResources* vulkan_shared_resources, VulkanRenderer* vulkan_renderer)
{
    vulkan_shared_resources->mesh_resources = new VulkanMeshResource[static_cast<size_t>(VulkanMeshType::COUNT)];
    for (size_t i = 0; i < static_cast<size_t>(VulkanMeshType::COUNT); ++i)
    {
        VulkanMeshResource& vulkan_mesh_resource = vulkan_shared_resources->mesh_resources[i];
        vulkan_mesh_resource.vertex_buffer = VK_NULL_HANDLE;
        vulkan_mesh_resource.vertex_allocation = VK_NULL_HANDLE;
        vulkan_mesh_resource.index_buffer = VK_NULL_HANDLE;
        vulkan_mesh_resource.index_allocation = VK_NULL_HANDLE;
    }

    Geometry* geometry = geometry_loader_load_cube();
    if (!vulkan_shared_resources_init_mesh_resource_with_geometry(geometry, vulkan_renderer,
        &vulkan_shared_resources->mesh_resources[static_cast<size_t>(VulkanMeshType::Cube)]))
    {
        return false;
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

    if (!vulkan_shared_resources_init_frame_buffer(vulkan_shared_resources, vulkan_renderer))
    {
        vulkan_shared_resources_destroy(vulkan_shared_resources, vulkan_renderer);
        return nullptr;
    }

    if (!vulkan_shared_resources_init_mesh_resources(vulkan_shared_resources, vulkan_renderer))
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

VulkanMeshResource* vulkan_shared_resources_get_mesh(VulkanSharedResources* vulkan_shared_resources, VulkanMeshType vulkan_mesh_type)
{
    uint32_t vulkan_mesh_type_index = static_cast<uint32_t>(vulkan_mesh_type);
    if (vulkan_mesh_type_index >= static_cast<uint32_t>(VulkanMeshType::COUNT))
    {
        SDL_assert(false);
        return nullptr;
    }

    return &vulkan_shared_resources->mesh_resources[vulkan_mesh_type_index];
}
