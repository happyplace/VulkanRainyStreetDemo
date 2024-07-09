#include "MeshRenderer.h"

#include <array>

#include <SDL_assert.h>
#include <SDL_log.h>

#include <shaderc/shaderc.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "vk_mem_alloc.h"

#include "Game.h"
#include "VulkanRenderer.h"
#include "VulkanSharedResources.h"
#include "VulkanFrameResources.h"

struct MeshRenderer
{
    VkBuffer object_buffer = VK_NULL_HANDLE;
    VmaAllocation object_allocation = VK_NULL_HANDLE;
    VkDeviceSize object_alignment_size = 0;
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
    VkDescriptorSetLayout* descriptor_set_layouts = nullptr;
    VkDescriptorSet* descriptor_sets = nullptr;
    VkShaderModule vertex = VK_NULL_HANDLE;
    VkShaderModule fragment = VK_NULL_HANDLE;
};

constexpr uint32_t c_mesh_renderer_desriptor_count = 4;
constexpr uint32_t c_mesh_renderer_max_mesh_object = 500;

void mesh_renderer_destroy_object_buffer(MeshRenderer* mesh_renderer, Game* game)
{
    if (mesh_renderer->object_allocation != VK_NULL_HANDLE || mesh_renderer->object_buffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(game->vulkan_renderer->vma_allocator, mesh_renderer->object_buffer, mesh_renderer->object_allocation);

        mesh_renderer->object_allocation = VK_NULL_HANDLE;
        mesh_renderer->object_buffer = VK_NULL_HANDLE;
    }
}

void mesh_renderer_destroy_descriptor_set(MeshRenderer* mesh_renderer, Game* game)
{
    if (mesh_renderer->descriptor_sets)
    {
        delete[] mesh_renderer->descriptor_sets;
    }
    mesh_renderer->descriptor_sets = new VkDescriptorSet[c_mesh_renderer_desriptor_count];
    for (uint32_t i = 0; i < c_mesh_renderer_desriptor_count; ++i)
    {
        mesh_renderer->descriptor_sets[i] = VK_NULL_HANDLE;
    }

    if (mesh_renderer->descriptor_set_layouts)
    {
        for (uint32_t i = 0; i < c_mesh_renderer_desriptor_count; ++i)
        {
            if (mesh_renderer->descriptor_set_layouts[i])
            {
                vkDestroyDescriptorSetLayout(game->vulkan_renderer->device, mesh_renderer->descriptor_set_layouts[i], s_allocator);
            }
        }

        delete[] mesh_renderer->descriptor_set_layouts;
    }

    if (mesh_renderer->descriptor_pool)
    {
        vkDestroyDescriptorPool(game->vulkan_renderer->device, mesh_renderer->descriptor_pool, s_allocator);
    }
}

void mesh_renderer_destroy_compile_vertex_shader(MeshRenderer* mesh_renderer, Game* game)
{
    if (mesh_renderer->vertex != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(game->vulkan_renderer->device, mesh_renderer->vertex, s_allocator);
    }
}

void mesh_renderer_destroy_compile_fragment_shader(MeshRenderer* mesh_renderer, Game* game)
{
    if (mesh_renderer->fragment != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(game->vulkan_renderer->device, mesh_renderer->fragment, s_allocator);
    }
}

void mesh_renderer_destroy(MeshRenderer* mesh_renderer, struct Game* game)
{
    SDL_assert(mesh_renderer);

    mesh_renderer_destroy_compile_fragment_shader(mesh_renderer, game);
    mesh_renderer_destroy_compile_vertex_shader(mesh_renderer, game);
    mesh_renderer_destroy_descriptor_set(mesh_renderer, game);
    mesh_renderer_destroy_object_buffer(mesh_renderer, game);

    delete mesh_renderer;
}

bool mesh_renderer_init_object_buffer(MeshRenderer* mesh_renderer, Game* game)
{
    mesh_renderer->object_alignment_size = vulkan_renderer_calculate_uniform_buffer_size(game->vulkan_renderer, sizeof(Vulkan_MeshRendererObjectBuffer));

    VkBufferCreateInfo buffer_create_info;
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.pNext = nullptr;
    buffer_create_info.flags = 0;
    buffer_create_info.size = mesh_renderer->object_alignment_size * VULKAN_FRAME_RESOURCES_FRAME_RESOURCE_COUNT * c_mesh_renderer_max_mesh_object;
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

    VkResult result = vmaCreateBuffer(game->vulkan_renderer->vma_allocator, &buffer_create_info, &allocation_create_info,
        &mesh_renderer->object_buffer, &mesh_renderer->object_allocation, nullptr);

    return result == VK_SUCCESS;
}

bool mesh_renderer_init_descriptor_set(MeshRenderer* mesh_renderer, Game* game)
{
    constexpr uint32_t max_render_object_count = 1;

    std::array<VkDescriptorPoolSize, c_mesh_renderer_desriptor_count> descriptor_pool_size;
    descriptor_pool_size[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_pool_size[0].descriptorCount = 1;
    descriptor_pool_size[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descriptor_pool_size[1].descriptorCount = 1;
    descriptor_pool_size[2].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    descriptor_pool_size[2].descriptorCount = 1;
    descriptor_pool_size[3].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    descriptor_pool_size[3].descriptorCount = max_render_object_count;

    VkDescriptorPoolCreateInfo descriptor_pool_create_info;
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.pNext = nullptr;
    descriptor_pool_create_info.flags = 0;
    descriptor_pool_create_info.maxSets = static_cast<uint32_t>(descriptor_pool_size.size());
    descriptor_pool_create_info.poolSizeCount = static_cast<uint32_t>(descriptor_pool_size.size());
    descriptor_pool_create_info.pPoolSizes = descriptor_pool_size.data();

    VkResult result = vkCreateDescriptorPool(game->vulkan_renderer->device, &descriptor_pool_create_info, s_allocator, &mesh_renderer->descriptor_pool);
    if (result != VK_SUCCESS)
    {
        VK_ASSERT(result);
        return false;
    }

    mesh_renderer->descriptor_set_layouts = new VkDescriptorSetLayout[c_mesh_renderer_desriptor_count];
    for (uint32_t i = 0; i < c_mesh_renderer_desriptor_count; ++i)
    {
        mesh_renderer->descriptor_set_layouts[i] = VK_NULL_HANDLE;
    }

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info;
    descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.pNext = nullptr;
    descriptor_set_layout_create_info.flags = 0;
    descriptor_set_layout_create_info.bindingCount = 1;

    VkDescriptorSetLayoutBinding frame_buffer_descriptor_set_layout_binding;
    frame_buffer_descriptor_set_layout_binding.binding = 0;
    frame_buffer_descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    frame_buffer_descriptor_set_layout_binding.descriptorCount = 1;
    frame_buffer_descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    frame_buffer_descriptor_set_layout_binding.pImmutableSamplers = nullptr;

    descriptor_set_layout_create_info.pBindings = &frame_buffer_descriptor_set_layout_binding;

    result = vkCreateDescriptorSetLayout(game->vulkan_renderer->device, &descriptor_set_layout_create_info, s_allocator, &mesh_renderer->descriptor_set_layouts[0]);
    if (result != VK_SUCCESS)
    {
        VK_ASSERT(result);
        return false;
    }

    VkDescriptorSetLayoutBinding object_buffer_descriptor_set_layout_binding;
    object_buffer_descriptor_set_layout_binding.binding = 0;
    object_buffer_descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    object_buffer_descriptor_set_layout_binding.descriptorCount = 1;
    object_buffer_descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    object_buffer_descriptor_set_layout_binding.pImmutableSamplers = nullptr;

    descriptor_set_layout_create_info.pBindings = &object_buffer_descriptor_set_layout_binding;

    result = vkCreateDescriptorSetLayout(game->vulkan_renderer->device, &descriptor_set_layout_create_info, s_allocator, &mesh_renderer->descriptor_set_layouts[1]);
    if (result != VK_SUCCESS)
    {
        VK_ASSERT(result);
        return false;
    }

    VkDescriptorSetLayoutBinding sampler_descriptor_set_layout_binding;
    sampler_descriptor_set_layout_binding.binding = 0;
    sampler_descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    sampler_descriptor_set_layout_binding.descriptorCount = 1;
    sampler_descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    VkSampler linear_sampler = vulkan_shared_resources_get_sampler(game->shared_resources, VulkanSharedResourcesSamplerType::Linear);
    sampler_descriptor_set_layout_binding.pImmutableSamplers = &linear_sampler;

    descriptor_set_layout_create_info.pBindings = &sampler_descriptor_set_layout_binding;

    result = vkCreateDescriptorSetLayout(game->vulkan_renderer->device, &descriptor_set_layout_create_info, s_allocator, &mesh_renderer->descriptor_set_layouts[2]);
    if (result != VK_SUCCESS)
    {
        VK_ASSERT(result);
        return false;
    }

    VkDescriptorSetLayoutBinding sampled_image_descriptor_set_layout_binding;
    sampled_image_descriptor_set_layout_binding.binding = 0;
    sampled_image_descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    sampled_image_descriptor_set_layout_binding.descriptorCount = max_render_object_count;
    sampled_image_descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    sampled_image_descriptor_set_layout_binding.pImmutableSamplers = nullptr;

    descriptor_set_layout_create_info.pBindings = &sampled_image_descriptor_set_layout_binding;

    result = vkCreateDescriptorSetLayout(game->vulkan_renderer->device, &descriptor_set_layout_create_info, s_allocator, &mesh_renderer->descriptor_set_layouts[3]);
    if (result != VK_SUCCESS)
    {
        VK_ASSERT(result);
        return false;
    }

    mesh_renderer->descriptor_sets = new VkDescriptorSet[c_mesh_renderer_desriptor_count];
    for (uint32_t i = 0; i < c_mesh_renderer_desriptor_count; ++i)
    {
        mesh_renderer->descriptor_sets[i] = VK_NULL_HANDLE;
    }

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info;
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = nullptr;
    descriptor_set_allocate_info.descriptorPool = mesh_renderer->descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = static_cast<uint32_t>(c_mesh_renderer_desriptor_count);
    descriptor_set_allocate_info.pSetLayouts = mesh_renderer->descriptor_set_layouts;

    result = vkAllocateDescriptorSets(game->vulkan_renderer->device, &descriptor_set_allocate_info, mesh_renderer->descriptor_sets);
    if (result != VK_SUCCESS)
    {
        VK_ASSERT(result);
        return false;
    }

    VkDescriptorBufferInfo frame_buffer_descriptor_buffer_info;
    frame_buffer_descriptor_buffer_info.buffer = game->shared_resources->frame_buffer;
    frame_buffer_descriptor_buffer_info.offset = 0;
    frame_buffer_descriptor_buffer_info.range= sizeof(Vulkan_FrameBuffer);

    VkWriteDescriptorSet frame_buffer_write_descriptor_set;
    frame_buffer_write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    frame_buffer_write_descriptor_set.pNext = nullptr;
    frame_buffer_write_descriptor_set.dstSet = mesh_renderer->descriptor_sets[0];
    frame_buffer_write_descriptor_set.dstBinding = 0;
    frame_buffer_write_descriptor_set.dstArrayElement = 0;
    frame_buffer_write_descriptor_set.descriptorCount = 1;
    frame_buffer_write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    frame_buffer_write_descriptor_set.pBufferInfo = &frame_buffer_descriptor_buffer_info;
    frame_buffer_write_descriptor_set.pImageInfo = nullptr;
    frame_buffer_write_descriptor_set.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(game->vulkan_renderer->device, 1, &frame_buffer_write_descriptor_set, 0, nullptr);

    VkDescriptorBufferInfo object_buffer_descriptor_buffer_info;
    object_buffer_descriptor_buffer_info.buffer = mesh_renderer->object_buffer;
    object_buffer_descriptor_buffer_info.offset = 0;
    object_buffer_descriptor_buffer_info.range = sizeof(Vulkan_MeshRendererObjectBuffer);

    VkWriteDescriptorSet object_buffer_write_descriptor_set;
    object_buffer_write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    object_buffer_write_descriptor_set.pNext = nullptr;
    object_buffer_write_descriptor_set.dstSet = mesh_renderer->descriptor_sets[1];
    object_buffer_write_descriptor_set.dstBinding = 0;
    object_buffer_write_descriptor_set.dstArrayElement = 0;
    object_buffer_write_descriptor_set.descriptorCount = 1;
    object_buffer_write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    object_buffer_write_descriptor_set.pBufferInfo = &object_buffer_descriptor_buffer_info;
    object_buffer_write_descriptor_set.pImageInfo = nullptr;
    object_buffer_write_descriptor_set.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(game->vulkan_renderer->device, 1, &object_buffer_write_descriptor_set, 0, nullptr);

    return true;
}

bool mesh_renderer_init_compile_vertex_shader(MeshRenderer* mesh_renderer, Game* game)
{
    shaderc_compile_options_t compile_options = shaderc_compile_options_initialize();
    if (compile_options == nullptr)
    {
        SDL_assert(false);
        return false;
    }

    bool result = vulkan_renderer_compile_shader(
        game->vulkan_renderer,
        "data/MeshShader.vert",
        compile_options,
        shaderc_glsl_vertex_shader,
        &mesh_renderer->vertex);

    shaderc_compile_options_release(compile_options);

    return result;
}

bool mesh_renderer_init_compile_fragment_shader(MeshRenderer* mesh_renderer, Game* game)
{
    shaderc_compile_options_t compile_options = shaderc_compile_options_initialize();
    if (compile_options == nullptr)
    {
        SDL_assert(false);
        return false;
    }

    bool result = vulkan_renderer_compile_shader(
        game->vulkan_renderer,
        "data/MeshShader.frag",
        compile_options,
        shaderc_glsl_fragment_shader,
        &mesh_renderer->fragment);

    shaderc_compile_options_release(compile_options);

    return result;
}

MeshRenderer* mesh_renderer_init(struct Game* game)
{
    MeshRenderer* mesh_renderer = new MeshRenderer();

    if (!mesh_renderer_init_object_buffer(mesh_renderer, game))
    {
        mesh_renderer_destroy(mesh_renderer, game);
        return nullptr;
    }

    if (!mesh_renderer_init_descriptor_set(mesh_renderer, game))
    {
        mesh_renderer_destroy(mesh_renderer, game);
        return nullptr;
    }

    if (!mesh_renderer_init_compile_vertex_shader(mesh_renderer, game))
    {
        mesh_renderer_destroy(mesh_renderer, game);
        return nullptr;
    }

    if (!mesh_renderer_init_compile_fragment_shader(mesh_renderer, game))
    {
        mesh_renderer_destroy(mesh_renderer, game);
        return nullptr;
    }

    return mesh_renderer;
}
