#include "VulkanSharedResources.h"

#include "ftl/task_scheduler.h"

#include <SDL_assert.h>

#include <cstring>
#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"

#include "VulkanRenderer.h"
#include "VulkanFrameResources.h"
#include "GeometryLoader.h"
#include "RenderDefines.h"

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

    vulkan_shared_resources_destroy_worker_thread();
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
    vulkan_shared_resources->frame_alignment_size = vulkan_renderer_calculate_uniform_buffer_size(vulkan_renderer, sizeof(MeshRenderer_FrameBuffer));

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

void vulkan_shared_resources_transfer_buffer_task(ftl::TaskScheduler* /*task_scheduler*/, void* arg)
{
    VulkanResourceTransferBufferParams* params = reinterpret_cast<VulkanResourceTransferBufferParams*>(arg);

    VkMemoryPropertyFlags memory_property_flags;
    vmaGetAllocationMemoryProperties(
        params->vulkan_renderer->vma_allocator, 
        params->allocation, &memory_property_flags);

    if (memory_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        void* mapped_data;
        VkResult result = vmaMapMemory(params->vulkan_renderer->vma_allocator, params->allocation, &mapped_data);
        VK_ASSERT(result);
        if (result != VK_SUCCESS)
        {
            delete params;
            return;
        }

        memcpy(mapped_data, params->data, params->data_size);
        if (!(memory_property_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        {
            vmaFlushAllocation(params->vulkan_renderer->vma_allocator, params->allocation, 0, VK_WHOLE_SIZE);
        }
        vmaUnmapMemory(params->vulkan_renderer->vma_allocator, params->allocation);

        params->vulkan_mesh_resource->resource_loaded_count.fetch_add(1);

        delete params;
    }
    else
    {
        vulkan_shared_resources_add_transfer_buffer_params(params);
    }
}

struct CopyGeometryToGpuParams
{
    Geometry* geometry = nullptr;
    VulkanRenderer* vulkan_renderer = nullptr;
    VulkanMeshResource* vulkan_mesh_resource = nullptr;
};

void vulkan_shared_resources_copy_geometry_to_gpu_task(ftl::TaskScheduler* task_scheduler, void* arg)
{
    CopyGeometryToGpuParams* params = reinterpret_cast<CopyGeometryToGpuParams*>(arg);

    SDL_assert(params->geometry);
    if (params->geometry == nullptr)
    {
        delete params;
        SDL_assert(false);
        return;
    }

    VkDeviceSize vertex_total_size = sizeof(Vertex) * params->geometry->vertex_count;

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

    VkResult result = vmaCreateBuffer(params->vulkan_renderer->vma_allocator, &vertex_buffer_create_info, &vertex_allocation_create_info,
        &params->vulkan_mesh_resource->vertex_buffer, &params->vulkan_mesh_resource->vertex_allocation, nullptr);

    VK_ASSERT(result);
    if (result == VK_SUCCESS)
    {
        VulkanResourceTransferBufferParams* vertex_transfer_buffer_params = new VulkanResourceTransferBufferParams();
        vertex_transfer_buffer_params->vulkan_renderer = params->vulkan_renderer;
        vertex_transfer_buffer_params->buffer = params->vulkan_mesh_resource->vertex_buffer;
        vertex_transfer_buffer_params->allocation = params->vulkan_mesh_resource->vertex_allocation;
        vertex_transfer_buffer_params->data = geometry_get_vertex(params->geometry);
        vertex_transfer_buffer_params->data_size = static_cast<size_t>(vertex_total_size);
        vertex_transfer_buffer_params->vulkan_mesh_resource = params->vulkan_mesh_resource;

        ftl::Task task = { vulkan_shared_resources_transfer_buffer_task, vertex_transfer_buffer_params };
        task_scheduler->AddTask(task, ftl::TaskPriority::Normal);
    }

    VkDeviceSize index_total_size = sizeof(uint32_t) * params->geometry->index_count;

    params->vulkan_mesh_resource->index_count = static_cast<uint32_t>(params->geometry->index_count);

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

    result = vmaCreateBuffer(params->vulkan_renderer->vma_allocator, &index_buffer_create_info, &index_allocation_create_info,
        &params->vulkan_mesh_resource->index_buffer, &params->vulkan_mesh_resource->index_allocation, nullptr);

    VK_ASSERT(result);
    if (result == VK_SUCCESS)
    {
        VulkanResourceTransferBufferParams* vertex_transfer_buffer_params = new VulkanResourceTransferBufferParams();
        vertex_transfer_buffer_params->vulkan_renderer = params->vulkan_renderer;
        vertex_transfer_buffer_params->buffer = params->vulkan_mesh_resource->index_buffer;
        vertex_transfer_buffer_params->allocation = params->vulkan_mesh_resource->index_allocation;
        vertex_transfer_buffer_params->data = geometry_get_index(params->geometry);
        vertex_transfer_buffer_params->data_size = static_cast<size_t>(index_total_size);
        vertex_transfer_buffer_params->vulkan_mesh_resource = params->vulkan_mesh_resource;

        ftl::Task task = { vulkan_shared_resources_transfer_buffer_task, vertex_transfer_buffer_params };
        task_scheduler->AddTask(task, ftl::TaskPriority::Normal);
    }
    
    delete params;
}

struct InitMeshResourcesParams
{
    VulkanSharedResources* vulkan_shared_resources = nullptr;
    VulkanRenderer* vulkan_renderer = nullptr;
};

void vulkan_shared_resources_load_cube_geometry_task(ftl::TaskScheduler* task_scheduler, void* arg)
{
    InitMeshResourcesParams* params = reinterpret_cast<InitMeshResourcesParams*>(arg);

    CopyGeometryToGpuParams* copy_geometry_to_gpu_params = new CopyGeometryToGpuParams();
    copy_geometry_to_gpu_params->geometry = geometry_loader_load_cube();
    copy_geometry_to_gpu_params->vulkan_renderer = params->vulkan_renderer;
    copy_geometry_to_gpu_params->vulkan_mesh_resource = 
        &params->vulkan_shared_resources->mesh_resources[static_cast<size_t>(VulkanMeshType::Cube)];

    ftl::Task task = { vulkan_shared_resources_copy_geometry_to_gpu_task, copy_geometry_to_gpu_params };
    task_scheduler->AddTask(task, ftl::TaskPriority::Normal);
}

void vulkan_shared_resources_init_mesh_resources_task(ftl::TaskScheduler* task_scheduler, void* arg)
{
    InitMeshResourcesParams* params = reinterpret_cast<InitMeshResourcesParams*>(arg);
    
    constexpr uint32_t mesh_load_task_count = 1;
    ftl::Task mesh_load_tasks[mesh_load_task_count];

    mesh_load_tasks[0] = { vulkan_shared_resources_load_cube_geometry_task, params };

    ftl::WaitGroup wait_group(task_scheduler);
    task_scheduler->AddTasks(mesh_load_task_count, mesh_load_tasks, ftl::TaskPriority::High, &wait_group);
    wait_group.Wait();

    delete params;
}

void vulkan_shared_resources_init_task(ftl::TaskScheduler* task_scheduler, void* arg)
{
    VulkanSharedResourcesInitParams* params = reinterpret_cast<VulkanSharedResourcesInitParams*>(arg);
    params->vulkan_shared_resources = new VulkanSharedResources();

    ftl::Task init_worker_thread_task = { vulkan_shared_resources_init_worker_thread_task, params->vulkan_renderer };
    ftl::WaitGroup wait_group(task_scheduler);
    task_scheduler->AddTask(init_worker_thread_task, ftl::TaskPriority::High, &wait_group);

    if (!vulkan_shared_resources_init_samplers(params->vulkan_shared_resources, params->vulkan_renderer))
    {
        params->init_successful = false;
        return;
    }

    if (!vulkan_shared_resources_init_frame_buffer(params->vulkan_shared_resources, params->vulkan_renderer))
    {
        params->init_successful = false;
        return;
    }

    params->vulkan_shared_resources->mesh_resources = new VulkanMeshResource[static_cast<size_t>(VulkanMeshType::COUNT)];
    for (uint32_t i = 0; i < static_cast<uint32_t>(VulkanMeshType::COUNT); ++i)
    {
        VulkanMeshResource& vulkan_mesh_resource = params->vulkan_shared_resources->mesh_resources[i];

        vulkan_mesh_resource.resource_loaded_count.store(0, std::memory_order_release);
        vulkan_mesh_resource.vertex_buffer = VK_NULL_HANDLE;
        vulkan_mesh_resource.vertex_allocation = VK_NULL_HANDLE;
        vulkan_mesh_resource.index_buffer = VK_NULL_HANDLE;
        vulkan_mesh_resource.index_allocation = VK_NULL_HANDLE;
        vulkan_mesh_resource.index_count = 0;
    }

    InitMeshResourcesParams* init_mesh_resources_params = new InitMeshResourcesParams();
    init_mesh_resources_params->vulkan_renderer = params->vulkan_renderer;
    init_mesh_resources_params->vulkan_shared_resources = params->vulkan_shared_resources;

    ftl::Task init_mesh_resources_task = { vulkan_shared_resources_init_mesh_resources_task, init_mesh_resources_params };

    // worker thread task needs to be completed before we can trigger the resource loading tasks
    wait_group.Wait();

    task_scheduler->AddTask(init_mesh_resources_task, ftl::TaskPriority::High);

    params->init_successful = true;    
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

    VulkanMeshResource* vulkan_mesh_resource = &vulkan_shared_resources->mesh_resources[vulkan_mesh_type_index];

    if (vulkan_mesh_resource->resource_loaded_count.load(std::memory_order_release) < 2)
    {
        return nullptr;
    }

    return &vulkan_shared_resources->mesh_resources[vulkan_mesh_type_index];
}

