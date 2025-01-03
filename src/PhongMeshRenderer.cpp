#include "PhongMeshRenderer.h"

#include <array>
#include <string>

#include <SDL_assert.h>
#include <SDL_log.h>

#include <cstring>
#include <shaderc/shaderc.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "DirectXMath.h"
#include "imgui.h"
#include "vk_mem_alloc.h"

#include "Game.h"
#include "VulkanRenderer.h"
#include "VulkanSharedResources.h"
#include "VulkanFrameResources.h"
#include "RenderDefines.h"

struct PhongMeshRenderer
{
    VkBuffer object_buffer = VK_NULL_HANDLE;
    VmaAllocation object_allocation = VK_NULL_HANDLE;
    VkDeviceSize object_alignment_size = 0;
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
    VkDescriptorSetLayout* descriptor_set_layouts = nullptr;
    VkDescriptorSet* descriptor_sets = nullptr;
    VkShaderModule vertex = VK_NULL_HANDLE;
    VkShaderModule fragment = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
};

constexpr uint32_t c_mesh_renderer_desriptor_count = 4;
constexpr uint32_t c_mesh_renderer_max_mesh_object = 500;
const char* mesh_renderer_vertex_entry = "vs";
const char* mesh_renderer_fragment_entry = "fs";

constexpr uint32_t c_frame_buffer_directional_light_count = 1;
constexpr uint32_t c_frame_buffer_point_light_count = 2;
constexpr uint32_t c_frame_buffer_spot_light_count = 1;

static std::array<PointLight, c_frame_buffer_point_light_count> s_phong_mesh_renderer_point_lights;
static std::array<DirectionalLight, c_frame_buffer_directional_light_count> s_phong_mesh_renderer_directional_lights;
static std::array<SpotLight, c_frame_buffer_spot_light_count> s_phong_mesh_renderer_spot_lights;
static DirectX::XMFLOAT3 s_phong_mesh_renderer_ambient_light = { 0.25f, 0.25f, 0.25f };

void phong_mesh_renderer_destroy_object_buffer(PhongMeshRenderer* mesh_renderer, Game* game)
{
    if (mesh_renderer->object_allocation != VK_NULL_HANDLE || mesh_renderer->object_buffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(game->vulkan_renderer->vma_allocator, mesh_renderer->object_buffer, mesh_renderer->object_allocation);

        mesh_renderer->object_allocation = VK_NULL_HANDLE;
        mesh_renderer->object_buffer = VK_NULL_HANDLE;
    }
}

void phong_mesh_renderer_destroy_descriptor_set(PhongMeshRenderer* mesh_renderer, Game* game)
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

void phong_mesh_renderer_destroy_compile_vertex_shader(PhongMeshRenderer* mesh_renderer, Game* game)
{
    if (mesh_renderer->vertex != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(game->vulkan_renderer->device, mesh_renderer->vertex, s_allocator);
    }
}

void phong_mesh_renderer_destroy_compile_fragment_shader(PhongMeshRenderer* mesh_renderer, Game* game)
{
    if (mesh_renderer->fragment != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(game->vulkan_renderer->device, mesh_renderer->fragment, s_allocator);
    }
}

void phong_mesh_renderer_destroy_pipeline_layout(PhongMeshRenderer* mesh_renderer, Game* game)
{
    if (mesh_renderer->pipeline_layout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(game->vulkan_renderer->device, mesh_renderer->pipeline_layout, s_allocator);
    }
}

void phong_mesh_renderer_destroy_pipeline(PhongMeshRenderer* mesh_renderer, Game* game)
{
    if (mesh_renderer->pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(game->vulkan_renderer->device, mesh_renderer->pipeline, s_allocator);
    }
}

void phong_mesh_renderer_destroy(PhongMeshRenderer* mesh_renderer, struct Game* game)
{
    SDL_assert(mesh_renderer);

    vulkan_renderer_wait_device_idle(game->vulkan_renderer);

    phong_mesh_renderer_destroy_pipeline(mesh_renderer, game);
    phong_mesh_renderer_destroy_pipeline_layout(mesh_renderer, game);
    phong_mesh_renderer_destroy_compile_fragment_shader(mesh_renderer, game);
    phong_mesh_renderer_destroy_compile_vertex_shader(mesh_renderer, game);
    phong_mesh_renderer_destroy_descriptor_set(mesh_renderer, game);
    phong_mesh_renderer_destroy_object_buffer(mesh_renderer, game);

    delete mesh_renderer;
}

bool phong_mesh_renderer_init_object_buffer(PhongMeshRenderer* mesh_renderer, Game* game)
{
    mesh_renderer->object_alignment_size = vulkan_renderer_calculate_uniform_buffer_size(game->vulkan_renderer, sizeof(ObjectBuffer));

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
    allocation_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
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

bool phong_mesh_renderer_init_descriptor_set(PhongMeshRenderer* mesh_renderer, Game* game)
{
    constexpr uint32_t max_render_object_count = 1;

    std::array<VkDescriptorPoolSize, c_mesh_renderer_desriptor_count> descriptor_pool_size;
    descriptor_pool_size[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
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
    frame_buffer_descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
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
    frame_buffer_descriptor_buffer_info.range = sizeof(MeshRenderer_FrameBuffer);

    VkWriteDescriptorSet frame_buffer_write_descriptor_set;
    frame_buffer_write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    frame_buffer_write_descriptor_set.pNext = nullptr;
    frame_buffer_write_descriptor_set.dstSet = mesh_renderer->descriptor_sets[0];
    frame_buffer_write_descriptor_set.dstBinding = 0;
    frame_buffer_write_descriptor_set.dstArrayElement = 0;
    frame_buffer_write_descriptor_set.descriptorCount = 1;
    frame_buffer_write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    frame_buffer_write_descriptor_set.pBufferInfo = &frame_buffer_descriptor_buffer_info;
    frame_buffer_write_descriptor_set.pImageInfo = nullptr;
    frame_buffer_write_descriptor_set.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(game->vulkan_renderer->device, 1, &frame_buffer_write_descriptor_set, 0, nullptr);

    VkDescriptorBufferInfo object_buffer_descriptor_buffer_info;
    object_buffer_descriptor_buffer_info.buffer = mesh_renderer->object_buffer;
    object_buffer_descriptor_buffer_info.offset = 0;
    object_buffer_descriptor_buffer_info.range = sizeof(ObjectBuffer);

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

bool phong_mesh_renderer_init_compile_vertex_shader(PhongMeshRenderer* mesh_renderer, Game* game)
{
    shaderc_compile_options_t compile_options = shaderc_compile_options_initialize();
    if (compile_options == nullptr)
    {
        SDL_assert(false);
        return false;
    }

    shaderc_compile_options_set_source_language(compile_options, shaderc_source_language_hlsl);

    bool result = vulkan_renderer_compile_shader(
        game->vulkan_renderer,
        "data/PhongMeshShader.hlsl",
        compile_options,
        shaderc_glsl_vertex_shader,
        mesh_renderer_vertex_entry,
        &mesh_renderer->vertex);

    shaderc_compile_options_release(compile_options);

    return result;
}

bool phong_mesh_renderer_init_compile_fragment_shader(PhongMeshRenderer* mesh_renderer, Game* game)
{
    shaderc_compile_options_t compile_options = shaderc_compile_options_initialize();
    if (compile_options == nullptr)
    {
        SDL_assert(false);
        return false;
    }

    shaderc_compile_options_set_source_language(compile_options, shaderc_source_language_hlsl);

    const char* num_directional_lights = "NUM_DIRECTIONAL_LIGHTS";
    std::string num_directional_lights_value = std::to_string(c_frame_buffer_directional_light_count);
    shaderc_compile_options_add_macro_definition(compile_options,
        num_directional_lights, strlen(num_directional_lights),
        num_directional_lights_value.c_str(), static_cast<size_t>(num_directional_lights_value.size()));

    const char* num_point_lights = "NUM_POINT_LIGHTS";
    std::string num_point_lights_value = std::to_string(c_frame_buffer_point_light_count);
    shaderc_compile_options_add_macro_definition(compile_options,
        num_point_lights, strlen(num_point_lights),
        num_point_lights_value.c_str(), static_cast<size_t>(num_point_lights_value.size()));

    const char* num_spot_lights = "NUM_SPOT_LIGHTS";
    std::string num_spot_lights_value = std::to_string(c_frame_buffer_spot_light_count);
    shaderc_compile_options_add_macro_definition(compile_options,
        num_spot_lights, strlen(num_spot_lights),
        num_spot_lights_value.c_str(), static_cast<size_t>(num_spot_lights_value.size()));

    bool result = vulkan_renderer_compile_shader(
        game->vulkan_renderer,
        "data/PhongMeshShader.hlsl",
        compile_options,
        shaderc_glsl_fragment_shader,
        mesh_renderer_fragment_entry,
        &mesh_renderer->fragment);

    shaderc_compile_options_release(compile_options);

    return result;
}

bool phong_mesh_renderer_init_pipeline_layout(PhongMeshRenderer* mesh_renderer, Game* game)
{
    VkPipelineLayoutCreateInfo pipeline_layout_create_info;
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.pNext = nullptr;
    pipeline_layout_create_info.flags = 0;
    pipeline_layout_create_info.setLayoutCount = c_mesh_renderer_desriptor_count;
    pipeline_layout_create_info.pSetLayouts = mesh_renderer->descriptor_set_layouts;
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges = nullptr;

    VkResult result = vkCreatePipelineLayout(game->vulkan_renderer->device, &pipeline_layout_create_info, s_allocator, &mesh_renderer->pipeline_layout);
    return result == VK_SUCCESS;
}

bool phong_mesh_renderer_init_pipeline(PhongMeshRenderer* mesh_renderer, Game* game)
{
    std::array<VkPipelineShaderStageCreateInfo, 2> pipeline_shader_stage_create_infos;
    pipeline_shader_stage_create_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_shader_stage_create_infos[0].pNext = nullptr;
    pipeline_shader_stage_create_infos[0].flags = 0;
    pipeline_shader_stage_create_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    pipeline_shader_stage_create_infos[0].module = mesh_renderer->vertex;
    pipeline_shader_stage_create_infos[0].pName = mesh_renderer_vertex_entry;
    pipeline_shader_stage_create_infos[0].pSpecializationInfo = nullptr;

    pipeline_shader_stage_create_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_shader_stage_create_infos[1].pNext = nullptr;
    pipeline_shader_stage_create_infos[1].flags = 0;
    pipeline_shader_stage_create_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    pipeline_shader_stage_create_infos[1].module = mesh_renderer->fragment;
    pipeline_shader_stage_create_infos[1].pName = mesh_renderer_fragment_entry;
    pipeline_shader_stage_create_infos[1].pSpecializationInfo = nullptr;

    std::array<VkVertexInputAttributeDescription, 3> vertex_input_attribute_descriptions;
    vertex_input_attribute_descriptions[0].location = 0;
    vertex_input_attribute_descriptions[0].binding = 0;
    vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_input_attribute_descriptions[0].offset = offsetof(Vertex, position);

    vertex_input_attribute_descriptions[1].location = 1;
    vertex_input_attribute_descriptions[1].binding = 0;
    vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_input_attribute_descriptions[1].offset = offsetof(Vertex, normal);

    vertex_input_attribute_descriptions[2].location = 2;
    vertex_input_attribute_descriptions[2].binding = 0;
    vertex_input_attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    vertex_input_attribute_descriptions[2].offset = offsetof(Vertex, texture);

    VkVertexInputBindingDescription vertex_input_binding_description;
    vertex_input_binding_description.binding = 0;
    vertex_input_binding_description.stride = sizeof(Vertex);
    vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info;
    pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipeline_vertex_input_state_create_info.pNext = nullptr;
    pipeline_vertex_input_state_create_info.flags = 0;
    pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
    pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_input_binding_description;
    pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_input_attribute_descriptions.size());
    pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_input_attribute_descriptions.data();

    VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info;
    pipeline_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipeline_input_assembly_state_create_info.pNext = nullptr;
    pipeline_input_assembly_state_create_info.flags = 0;
    pipeline_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipeline_input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

    VkPipelineTessellationStateCreateInfo pipeline_tessellation_state_create_info;
    pipeline_tessellation_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    pipeline_tessellation_state_create_info.pNext = nullptr;
    pipeline_tessellation_state_create_info.flags = 0;
    pipeline_tessellation_state_create_info.patchControlPoints = 0;

    VkViewport viewport;
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = static_cast<float>(game->vulkan_renderer->swapchain_width);
    viewport.height = static_cast<float>(game->vulkan_renderer->swapchain_height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissoring;
    scissoring.offset.x = 0;
    scissoring.offset.y = 0;
    scissoring.extent.width = static_cast<float>(game->vulkan_renderer->swapchain_width);
    scissoring.extent.height = static_cast<float>(game->vulkan_renderer->swapchain_height);

    VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info;
    pipeline_viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipeline_viewport_state_create_info.pNext = nullptr;
    pipeline_viewport_state_create_info.flags = 0;
    pipeline_viewport_state_create_info.viewportCount = 1;
    pipeline_viewport_state_create_info.pViewports = &viewport;
    pipeline_viewport_state_create_info.scissorCount = 1;
    pipeline_viewport_state_create_info.pScissors = &scissoring;

    constexpr bool wireframe = false;

    VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info;
    pipeline_rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipeline_rasterization_state_create_info.pNext = nullptr;
    pipeline_rasterization_state_create_info.flags = 0;
    pipeline_rasterization_state_create_info.depthClampEnable = VK_FALSE;
    pipeline_rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    pipeline_rasterization_state_create_info.polygonMode = wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    pipeline_rasterization_state_create_info.cullMode = wireframe ? VK_CULL_MODE_NONE : VK_CULL_MODE_FRONT_BIT;
    pipeline_rasterization_state_create_info.frontFace =  VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipeline_rasterization_state_create_info.depthBiasEnable = VK_FALSE;
    pipeline_rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
    pipeline_rasterization_state_create_info.depthBiasClamp = 0.0f;
    pipeline_rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;
    pipeline_rasterization_state_create_info.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info;
    pipeline_multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipeline_multisample_state_create_info.pNext = nullptr;
    pipeline_multisample_state_create_info.flags = 0;
    pipeline_multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipeline_multisample_state_create_info.sampleShadingEnable = VK_FALSE;
    pipeline_multisample_state_create_info.minSampleShading = 0;
    pipeline_multisample_state_create_info.pSampleMask = nullptr;
    pipeline_multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
    pipeline_multisample_state_create_info.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state;
    pipeline_color_blend_attachment_state.blendEnable = VK_FALSE;
    pipeline_color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    pipeline_color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pipeline_color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    pipeline_color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    pipeline_color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    pipeline_color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
    pipeline_color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info;
    pipeline_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipeline_color_blend_state_create_info.pNext = nullptr;
    pipeline_color_blend_state_create_info.flags = 0;
    pipeline_color_blend_state_create_info.logicOpEnable = VK_FALSE;
    pipeline_color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
    pipeline_color_blend_state_create_info.attachmentCount = 1;
    pipeline_color_blend_state_create_info.pAttachments = &pipeline_color_blend_attachment_state;
    pipeline_color_blend_state_create_info.blendConstants[0] = 0.0f;
    pipeline_color_blend_state_create_info.blendConstants[1] = 0.0f;
    pipeline_color_blend_state_create_info.blendConstants[2] = 0.0f;
    pipeline_color_blend_state_create_info.blendConstants[3] = 0.0f;

    std::array<VkDynamicState,2> dynamic_state;
    dynamic_state[0] = VK_DYNAMIC_STATE_VIEWPORT;
    dynamic_state[1] = VK_DYNAMIC_STATE_SCISSOR;

    VkPipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info;
    pipeline_dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pipeline_dynamic_state_create_info.pNext = nullptr;
    pipeline_dynamic_state_create_info.flags = 0;
    pipeline_dynamic_state_create_info.dynamicStateCount = static_cast<uint32_t>(dynamic_state.size());
    pipeline_dynamic_state_create_info.pDynamicStates = dynamic_state.data();

    VkStencilOpState stencil_op_state_front;
    stencil_op_state_front.failOp = VK_STENCIL_OP_KEEP;
    stencil_op_state_front.passOp = VK_STENCIL_OP_REPLACE;
    stencil_op_state_front.depthFailOp = VK_STENCIL_OP_KEEP;
    stencil_op_state_front.compareOp = VK_COMPARE_OP_LESS;
    stencil_op_state_front.compareMask = ~0;
    stencil_op_state_front.writeMask = ~0;
    stencil_op_state_front.reference = 0;

    VkStencilOpState stencil_op_state_back;
    stencil_op_state_back.failOp = VK_STENCIL_OP_KEEP;
    stencil_op_state_back.passOp = VK_STENCIL_OP_KEEP;
    stencil_op_state_back.depthFailOp = VK_STENCIL_OP_KEEP;
    stencil_op_state_back.compareOp = VK_COMPARE_OP_NEVER;
    stencil_op_state_back.compareMask = ~0;
    stencil_op_state_back.writeMask = ~0;
    stencil_op_state_back.reference = 0;

    VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info;
    pipeline_depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipeline_depth_stencil_state_create_info.pNext = nullptr;
    pipeline_depth_stencil_state_create_info.flags = 0;
    pipeline_depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
    pipeline_depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
    pipeline_depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
    pipeline_depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    pipeline_depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
    pipeline_depth_stencil_state_create_info.front = stencil_op_state_front;
    pipeline_depth_stencil_state_create_info.back = stencil_op_state_back;
    pipeline_depth_stencil_state_create_info.minDepthBounds = 0.0f;
    pipeline_depth_stencil_state_create_info.maxDepthBounds = 1.0f;

    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info;
    graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphics_pipeline_create_info.pNext = nullptr;
    graphics_pipeline_create_info.flags = 0;
    graphics_pipeline_create_info.stageCount = static_cast<uint32_t>(pipeline_shader_stage_create_infos.size());
    graphics_pipeline_create_info.pStages = pipeline_shader_stage_create_infos.data();
    graphics_pipeline_create_info.pVertexInputState = &pipeline_vertex_input_state_create_info;
    graphics_pipeline_create_info.pInputAssemblyState = &pipeline_input_assembly_state_create_info;
    graphics_pipeline_create_info.pTessellationState = &pipeline_tessellation_state_create_info;
    graphics_pipeline_create_info.pViewportState = &pipeline_viewport_state_create_info;
    graphics_pipeline_create_info.pRasterizationState = &pipeline_rasterization_state_create_info;
    graphics_pipeline_create_info.pMultisampleState = &pipeline_multisample_state_create_info;
    graphics_pipeline_create_info.pDepthStencilState = &pipeline_depth_stencil_state_create_info;
    graphics_pipeline_create_info.pColorBlendState = &pipeline_color_blend_state_create_info;
    graphics_pipeline_create_info.pDynamicState = &pipeline_dynamic_state_create_info;
    graphics_pipeline_create_info.layout = mesh_renderer->pipeline_layout;
    graphics_pipeline_create_info.renderPass = game->vulkan_renderer->render_pass;
    graphics_pipeline_create_info.subpass = 0;
    graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    graphics_pipeline_create_info.basePipelineIndex = 0;

    VkResult result = vkCreateGraphicsPipelines(
        game->vulkan_renderer->device,
        VK_NULL_HANDLE,
        1,
        &graphics_pipeline_create_info,
        s_allocator,
        &mesh_renderer->pipeline);

    VK_ASSERT(result);

    return result == VK_SUCCESS;
}

PhongMeshRenderer* phong_mesh_renderer_init(struct Game* game)
{
    PhongMeshRenderer* mesh_renderer = new PhongMeshRenderer();

    if (!phong_mesh_renderer_init_object_buffer(mesh_renderer, game))
    {
        phong_mesh_renderer_destroy(mesh_renderer, game);
        return nullptr;
    }

    if (!phong_mesh_renderer_init_descriptor_set(mesh_renderer, game))
    {
        phong_mesh_renderer_destroy(mesh_renderer, game);
        return nullptr;
    }

    if (!phong_mesh_renderer_init_compile_vertex_shader(mesh_renderer, game))
    {
        phong_mesh_renderer_destroy(mesh_renderer, game);
        return nullptr;
    }

    if (!phong_mesh_renderer_init_compile_fragment_shader(mesh_renderer, game))
    {
        phong_mesh_renderer_destroy(mesh_renderer, game);
        return nullptr;
    }

    if (!phong_mesh_renderer_init_pipeline_layout(mesh_renderer, game))
    {
        phong_mesh_renderer_destroy(mesh_renderer, game);
        return nullptr;
    }

    if (!phong_mesh_renderer_init_pipeline(mesh_renderer, game))
    {
        phong_mesh_renderer_destroy(mesh_renderer, game);
        return nullptr;
    }

    if (0 < s_phong_mesh_renderer_directional_lights.size())
    {
        s_phong_mesh_renderer_directional_lights[0].rotation_euler = { 90.0f, 50.0f, 0.0f };
        s_phong_mesh_renderer_directional_lights[0].strength = { 0.501961f, 0.380392f, 0.133333f };
    }

    if (0 < s_phong_mesh_renderer_point_lights.size())
    {
        s_phong_mesh_renderer_point_lights[0].position = DirectX::XMFLOAT3(-4.0f, 0.0f, -3.0f);
        s_phong_mesh_renderer_point_lights[0].strength = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);
        s_phong_mesh_renderer_point_lights[0].falloff_start = 5.0f;
        s_phong_mesh_renderer_point_lights[0].falloff_end = 5.0f;
    }
    if (1 < s_phong_mesh_renderer_point_lights.size())
    {
        s_phong_mesh_renderer_point_lights[1].position = DirectX::XMFLOAT3(4.0f, 0.0f, -3.0f);
        s_phong_mesh_renderer_point_lights[1].strength = DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f);
        s_phong_mesh_renderer_point_lights[1].falloff_start = 5.0f;
        s_phong_mesh_renderer_point_lights[1].falloff_end = 5.0f;
    }

    if (0 < s_phong_mesh_renderer_spot_lights.size())
    {
        s_phong_mesh_renderer_spot_lights[0].rotation_euler = DirectX::XMFLOAT3(90.0f, 0.0f, 0.0f);
        s_phong_mesh_renderer_spot_lights[0].strength = DirectX::XMFLOAT3(0.023529f, 0.454902f, 0.086275f);
        s_phong_mesh_renderer_spot_lights[0].position = DirectX::XMFLOAT3(0.0f, 2.0f, 0.0f);
        s_phong_mesh_renderer_spot_lights[0].falloff_start = 25.f;
        s_phong_mesh_renderer_spot_lights[0].falloff_end = 30.f;
        s_phong_mesh_renderer_spot_lights[0].spot_power = 34.f;
    }

    return mesh_renderer;
}

uint32_t phong_mesh_renderer_get_object_buffer_offset(PhongMeshRenderer* mesh_renderer, struct FrameResource* frame_resource, uint32_t object_index)
{
    return static_cast<uint32_t>((c_mesh_renderer_max_mesh_object * mesh_renderer->object_alignment_size * frame_resource->index) + object_index);
}

void phong_mesh_renderer_imgui_draw()
{
    if (ImGui::Begin("Phong Renderer", nullptr, ImGuiWindowFlags_None))
    { 
        ImGui::InputFloat3("Ambient Light", &s_phong_mesh_renderer_ambient_light.x, "%.3f", ImGuiWindowFlags_None);

        ImGui::LabelText("", "Directional Lights");
        ImGui::PushID("Directional Lights");
        for (uint32_t i = 0; i < c_frame_buffer_directional_light_count; ++i)
        {
            ImGui::PushID(i);
            ImGui::ColorEdit3("strength", &s_phong_mesh_renderer_directional_lights[i].strength.x, ImGuiColorEditFlags_None);
            ImGui::InputFloat3("rotation", &s_phong_mesh_renderer_directional_lights[i].rotation_euler.x, "%.3f", ImGuiWindowFlags_None);
            ImGui::PopID();
        }
        ImGui::PopID();

        ImGui::LabelText("", "Point Lights");
        ImGui::PushID("Point Lights");
        for (uint32_t i = 0; i < c_frame_buffer_point_light_count; ++i)
        {
            ImGui::PushID(i);
            ImGui::ColorEdit3("strength", &s_phong_mesh_renderer_point_lights[i].strength.x, ImGuiColorEditFlags_None);
            ImGui::InputFloat3("Position", &s_phong_mesh_renderer_point_lights[i].position.x, "%.3f", ImGuiWindowFlags_None);
            ImGui::InputFloat("Falloff Start", &s_phong_mesh_renderer_point_lights[i].falloff_start, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_None);
            ImGui::InputFloat("Falloff End", &s_phong_mesh_renderer_point_lights[i].falloff_end, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_None);
            ImGui::PopID();
        }
        ImGui::PopID();

        ImGui::LabelText("", "Spot Lights");
        ImGui::PushID("Spot Lights");
        for (uint32_t i = 0; i < c_frame_buffer_spot_light_count; ++i)
        {
            ImGui::PushID(i);
            ImGui::ColorEdit3("strength", &s_phong_mesh_renderer_spot_lights[i].strength.x, ImGuiColorEditFlags_None);
            ImGui::InputFloat3("Position", &s_phong_mesh_renderer_spot_lights[i].position.x, "%.3f", ImGuiWindowFlags_None);
            ImGui::InputFloat3("rotation", &s_phong_mesh_renderer_spot_lights[i].rotation_euler.x, "%.3f", ImGuiWindowFlags_None);
            ImGui::InputFloat("Falloff Start", &s_phong_mesh_renderer_spot_lights[i].falloff_start, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_None);
            ImGui::InputFloat("Falloff End", &s_phong_mesh_renderer_spot_lights[i].falloff_end, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_None);
            ImGui::InputFloat("Spot Power", &s_phong_mesh_renderer_spot_lights[i].spot_power, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_None);
            ImGui::PopID();
        }
        ImGui::PopID();
    }
    ImGui::End();
}

void phong_mesh_renderer_render(PhongMeshRenderer* mesh_renderer, struct FrameResource* frame_resource, struct Game* game)
{
    using namespace DirectX;

    vkCmdBindPipeline(frame_resource->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_renderer->pipeline);

    VkViewport viewport;
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = static_cast<float>(game->vulkan_renderer->swapchain_width);
    viewport.height = static_cast<float>(game->vulkan_renderer->swapchain_height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(frame_resource->command_buffer, 0, 1, &viewport);

    VkRect2D scissor;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = static_cast<float>(game->vulkan_renderer->swapchain_width);
    scissor.extent.height = static_cast<float>(game->vulkan_renderer->swapchain_height);
    vkCmdSetScissor(frame_resource->command_buffer, 0, 1, &scissor);

    uint32_t frame_buffer_offset = vulkan_shared_resources_get_frame_buffer_offset(game->shared_resources, frame_resource);
    vkCmdBindDescriptorSets(frame_resource->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_renderer->pipeline_layout, 0, 1,
        &mesh_renderer->descriptor_sets[0], 1, &frame_buffer_offset);
    vkCmdBindDescriptorSets(frame_resource->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_renderer->pipeline_layout, 2, 1,
        &mesh_renderer->descriptor_sets[2], 0, nullptr);
    vkCmdBindDescriptorSets(frame_resource->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_renderer->pipeline_layout, 3, 1,
        &mesh_renderer->descriptor_sets[3], 0, nullptr);

    MeshRenderer_FrameBuffer frame_buffer;

    XMVECTOR position = XMVectorSet(0.0f, 1.0f, -2.0f, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    float width = static_cast<float>(game->vulkan_renderer->swapchain_width);
    float height = static_cast<float>(game->vulkan_renderer->swapchain_height);
    float aspect_ratio = width / height;

    XMMATRIX view = XMMatrixLookAtLH(position, target, up);
    XMMATRIX proj = XMMatrixPerspectiveFovLH(
        XMConvertToRadians(80.0f),
        aspect_ratio,
        0.1f,
        10000.0f);
    XMFLOAT4X4 proj_float;
    XMStoreFloat4x4(&proj_float, proj);
    proj_float.m[1][1] *= -1.0f; // flip y coordinate for vulkan
    proj = XMLoadFloat4x4(&proj_float);

    XMMATRIX view_proj = view * proj;
    view_proj = XMMatrixTranspose(view_proj);

    frame_buffer.ambient_light = XMFLOAT4(
        s_phong_mesh_renderer_ambient_light.x, 
        s_phong_mesh_renderer_ambient_light.y, 
        s_phong_mesh_renderer_ambient_light.z, 1.0f);

    XMStoreFloat3(&frame_buffer.eye_pos, position);

    uint32_t current_light_index = 0;

    for (DirectionalLight& directional_light : s_phong_mesh_renderer_directional_lights)
    {
        if (current_light_index >= c_render_defines_max_lights)
        {
            SDL_assert(false);
            break;
        }

        DirectX::XMMATRIX rotation_matrix = DirectX::XMMatrixRotationRollPitchYaw(
            DirectX::XMConvertToRadians(directional_light.rotation_euler.x), 
            DirectX::XMConvertToRadians(directional_light.rotation_euler.y), 
            DirectX::XMConvertToRadians(directional_light.rotation_euler.z));

        XMVECTOR forward_light = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
        XMVECTOR direction = XMVector4Transform(forward_light, rotation_matrix);
        XMStoreFloat3(&frame_buffer.lights[current_light_index].direction, direction);

        frame_buffer.lights[current_light_index].strength = directional_light.strength;

        current_light_index++;
    }

    for (PointLight& point_light : s_phong_mesh_renderer_point_lights)
    {
        if (current_light_index >= c_render_defines_max_lights)
        {
            SDL_assert(false);
            break;
        }

        frame_buffer.lights[current_light_index].position = point_light.position;
        frame_buffer.lights[current_light_index].falloff_start = point_light.falloff_start;
        frame_buffer.lights[current_light_index].falloff_end = point_light.falloff_end;
        frame_buffer.lights[current_light_index].strength = point_light.strength;

        current_light_index++;
    }

    for (SpotLight& spot_light : s_phong_mesh_renderer_spot_lights)
    {
        if (current_light_index >= c_render_defines_max_lights)
        {
            SDL_assert(false);
            break;
        }

        DirectX::XMMATRIX rotation_matrix = DirectX::XMMatrixRotationRollPitchYaw(
            DirectX::XMConvertToRadians(spot_light.rotation_euler.x), 
            DirectX::XMConvertToRadians(spot_light.rotation_euler.y), 
            DirectX::XMConvertToRadians(spot_light.rotation_euler.z));

        XMVECTOR forward_light = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
        XMVECTOR direction = XMVector4Transform(forward_light, rotation_matrix);
        XMStoreFloat3(&frame_buffer.lights[current_light_index].direction, direction);

        frame_buffer.lights[current_light_index].strength = spot_light.strength;
        frame_buffer.lights[current_light_index].position = spot_light.position;
        frame_buffer.lights[current_light_index].falloff_start = spot_light.falloff_start;
        frame_buffer.lights[current_light_index].falloff_end = spot_light.falloff_end;
        frame_buffer.lights[current_light_index].spot_power = spot_light.spot_power;

        current_light_index++;
    }

    XMStoreFloat4x4(&frame_buffer.view_proj, view_proj);

    VK_ASSERT(vmaCopyMemoryToAllocation(
        game->vulkan_renderer->vma_allocator,
        &frame_buffer,
        game->shared_resources->frame_allocation,
        frame_buffer_offset,
        static_cast<VkDeviceSize>(sizeof(MeshRenderer_FrameBuffer))));

    {
        constexpr VulkanMeshType vulkan_mesh_type = VulkanMeshType::Cube;
        const VulkanMeshResource* vulkan_mesh_resource = vulkan_shared_resources_get_mesh(game->shared_resources, vulkan_mesh_type);
        if (vulkan_mesh_resource == nullptr)
        {
            return;
        }

        ObjectBuffer object_buffer;

        XMMATRIX world = XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixRotationRollPitchYaw(XMConvertToRadians(0.0f), XMConvertToRadians(0.0f), XMConvertToRadians(0.0f));
        world = XMMatrixTranspose(world);

        XMStoreFloat4x4(&object_buffer.world, world);

        object_buffer.material.diffuse_albedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        object_buffer.material.fresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05);
        object_buffer.material.shininess = 0.7f;

        uint32_t object_buffer_offset = phong_mesh_renderer_get_object_buffer_offset(mesh_renderer, frame_resource, 0);

        VK_ASSERT(vmaCopyMemoryToAllocation(
            game->vulkan_renderer->vma_allocator,
            &object_buffer,
            mesh_renderer->object_allocation,
            object_buffer_offset,
            static_cast<VkDeviceSize>(sizeof(ObjectBuffer))));

        vkCmdBindDescriptorSets(frame_resource->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_renderer->pipeline_layout, 1, 1,
            &mesh_renderer->descriptor_sets[1], 1, &object_buffer_offset);

        const VkDeviceSize vertex_offset = 0;
        vkCmdBindVertexBuffers(frame_resource->command_buffer, 0, 1, &vulkan_mesh_resource->vertex_buffer, &vertex_offset);
        vkCmdBindIndexBuffer(frame_resource->command_buffer, vulkan_mesh_resource->index_buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(frame_resource->command_buffer, vulkan_mesh_resource->index_count, 1, 0, 0, 0);
    }
}
