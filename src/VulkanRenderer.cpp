#include "VulkanRenderer.h"

#include <cstddef>
#include <cstdint>
#include <vector>

#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan.h>

#include <SDL_assert.h>
#include <SDL_log.h>
#include <SDL_vulkan.h>

#include "GameWindow.h"

#define VK_ASSERT(X) SDL_assert(VK_SUCCESS == X)

static VkAllocationCallbacks* s_allocator = nullptr;

#ifdef VK_DEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_renderer_debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT /*type*/,
    uint64_t /*object*/, size_t /*location*/, int32_t /*message_code*/, const char* layer_prefix, const char* message, void* /*user_data*/)
{
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Validation Layer: %s: %s", layer_prefix, message);
    }
    else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Validation Layer: %s: %s", layer_prefix, message);
    }
    else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Performance warning: %s: %s", layer_prefix, message);
    }
    else
    {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Validation Layer: Information: %s: %s", layer_prefix, message);
    }
    return VK_FALSE;
}
#endif // VK_DEBUG

bool vulkan_renderer_find_memory_by_flag_and_type(VulkanRenderer* vulkan_renderer, VkMemoryPropertyFlagBits memory_property_flag_bits, uint32_t memory_type_bits, uint32_t* out_memory_type_index)
{
    VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
    vkGetPhysicalDeviceMemoryProperties(vulkan_renderer->physical_device, &physical_device_memory_properties);

    for (uint32_t i = 0; i < physical_device_memory_properties.memoryTypeCount; ++i)
    {
        if ((memory_type_bits & (1<<i)) != 0)
        {
            if ((physical_device_memory_properties.memoryTypes[i].propertyFlags & memory_property_flag_bits) != 0)
            {
                (*out_memory_type_index) = i;
                return true;
            }
        }
    }

    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not find given memory flag (0x%08x) and type (0x%08x))", memory_property_flag_bits, memory_type_bits);
    return false;
}

bool vulkan_renderer_different_compute_and_graphics_queue(VulkanRenderer* vulkan_renderer)
{
    SDL_assert(vulkan_renderer);
    return vulkan_renderer->graphics_queue_index != vulkan_renderer->compute_queue_index;
}

bool vulkan_renderer_init_instance(GameWindow* game_window, VulkanRenderer* vulkan_renderer)
{
    uint32_t extension_count;
    if (!SDL_Vulkan_GetInstanceExtensions(game_window->window,&extension_count, nullptr))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "error getting vulkan instance extensions");
        return false;
    }

    std::vector<const char*> enabled_extension_names(extension_count);
    if (!SDL_Vulkan_GetInstanceExtensions(game_window->window, &extension_count, enabled_extension_names.data()))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "error getting vulkan instance extensions into array");
        return false;
    }

    uint32_t extension_properties_count;
    VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &extension_properties_count, nullptr));

    std::vector<VkExtensionProperties> instance_extension_properties(extension_properties_count);
    VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &extension_properties_count, instance_extension_properties.data()));

    uint32_t instance_layer_count;
    VK_ASSERT(vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr));

    std::vector<VkLayerProperties> instance_layer_properties(instance_layer_count);
    VK_ASSERT(vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layer_properties.data()));

    VkApplicationInfo application_info;
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pNext = nullptr;
    application_info.pApplicationName = "Vulkan Rainy Street Demo";
    application_info.pEngineName = "Vulkan Rainy Street Demo Engine";
    application_info.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
    application_info.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
    application_info.apiVersion = VK_API_VERSION_1_0;

    std::vector<const char*> enabled_layer_names;
#ifdef VK_DEBUG
    enabled_layer_names.push_back("VK_LAYER_KHRONOS_validation");
#endif // VK_DEBUG

    for (const char* layer_name : enabled_layer_names)
    {
        bool layer_available = false;

        for (const VkLayerProperties& layer_properties : instance_layer_properties)
        {
            if (strcmp(layer_name, layer_properties.layerName) == 0)
            {
                layer_available = true;
                break;
            }
        }

        if (!layer_available)
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "failed to init vulkan instance, layer requested but not supported: %s", layer_name);
            return false;
        }
    }

    VkInstanceCreateInfo instance_info;
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pNext = nullptr;
    instance_info.flags = 0;
    instance_info.pApplicationInfo = &application_info;
    instance_info.enabledLayerCount = static_cast<uint32_t>(enabled_layer_names.size());
    instance_info.ppEnabledLayerNames = enabled_layer_names.data();

#ifdef VK_DEBUG
    VkDebugReportCallbackCreateInfoEXT debug_report_callback_create_info;
    debug_report_callback_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    debug_report_callback_create_info.pNext = nullptr;
    debug_report_callback_create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    debug_report_callback_create_info.pfnCallback = vulkan_renderer_debug_callback;
    debug_report_callback_create_info.pUserData = nullptr;

    instance_info.pNext = &debug_report_callback_create_info;
#endif // VK_DEBUG

#ifdef VK_PORTABILITY
    instance_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

    enabled_extension_names.push_back("VK_KHR_portability_enumeration");
    enabled_extension_names.push_back("VK_KHR_get_physical_device_properties2");
#endif // VK_PORTABILITY

#ifdef VK_DEBUG
    for (const VkExtensionProperties& extension_properties : instance_extension_properties)
    {
        if (strcmp(extension_properties.extensionName, VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0)
        {
            enabled_extension_names.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
            break;
        }
    }
#endif // VK_DEBUG

    for (const char* requested_extension_name : enabled_extension_names)
    {
        bool extension_available = false;

        for (const VkExtensionProperties& extension_properties : instance_extension_properties)
        {
            if (strcmp(requested_extension_name, extension_properties.extensionName) == 0)
            {
                extension_available = true;
                break;
            }
        }

        if (!extension_available)
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "failed to init vulkan instance, extension requested but not supported: %s", requested_extension_name);
            return false;
        }
    }

    instance_info.enabledExtensionCount = static_cast<uint32_t>(enabled_extension_names.size());
    instance_info.ppEnabledExtensionNames = enabled_extension_names.data();

    VkResult result = vkCreateInstance(&instance_info, s_allocator, &vulkan_renderer->instance);
    if (result == VK_ERROR_LAYER_NOT_PRESENT)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "failed to create vulkan instance, layers requested:");
        for (const char* layer_name : enabled_layer_names)
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "\t%s", layer_name);
        }
    }
    else if (result == VK_ERROR_EXTENSION_NOT_PRESENT)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "failed to create vulkan instance, enabled extensions:");
        for (const char* extension_name : enabled_extension_names)
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "\t%s", extension_name);
        }
    }

    if (result != VK_SUCCESS)
    {
        VK_ASSERT(result);
        return false;
    }

    if (!SDL_Vulkan_CreateSurface(game_window->window, vulkan_renderer->instance, &vulkan_renderer->window_surface))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "failed to create surface for sdl window: %s", SDL_GetError());
        return false;
    }

#ifdef VK_DEBUG
    PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(vulkan_renderer->instance, "vkCreateDebugReportCallbackEXT");
    SDL_assert(vkCreateDebugReportCallbackEXT);

    VK_ASSERT(vkCreateDebugReportCallbackEXT(vulkan_renderer->instance, &debug_report_callback_create_info, s_allocator, &vulkan_renderer->debug_report_callback_ext));
#endif // VK_DEBUG

    return true;
}

bool vulkan_renderer_init_device(VulkanRenderer* vulkan_renderer)
{
    uint32_t physical_device_count = 0;
    VK_ASSERT(vkEnumeratePhysicalDevices(vulkan_renderer->instance, &physical_device_count, nullptr));

    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    VK_ASSERT(vkEnumeratePhysicalDevices(vulkan_renderer->instance, &physical_device_count, physical_devices.data()));

    int32_t graphics_queue_index = -1;
    int32_t compute_queue_index = -1;
    int8_t device_type_score = -1;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;

    for (uint32_t i = 0; i < physical_device_count; ++i)
    {
        VkPhysicalDeviceProperties device_properties;
        vkGetPhysicalDeviceProperties(physical_devices[i], &device_properties);

        // we're only dealing with Discrete or Integrated gpus, everything else is ignored
        // in the future we should just score all of the GPUs on their system and then pick
        // the one with the highest score, and also allow them to select which GPU to use
        if (device_properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
            device_properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        {
            continue;
        }

        uint32_t queue_family_property_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_property_count, nullptr);

        std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_property_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_property_count, queue_family_properties.data());

        int32_t graphics_index = -1;
        int32_t compute_index = -1;

        for (uint32_t k = 0; k < queue_family_property_count; ++k)
        {
            VkQueueFamilyProperties& family_property = queue_family_properties[k];

            VkBool32 supports_present = false;
            VK_ASSERT(vkGetPhysicalDeviceSurfaceSupportKHR(physical_devices[i], k, vulkan_renderer->window_surface, &supports_present));

            // TODO: we take the first queue that supports the graphics or compute bit and don't care about any others, should we be doing more?
            // at the very least should we be priortizing queues that susport both graphics and compute so we don't need to transfer things back
            // and forward

            if (graphics_index <= -1 && (family_property.queueFlags & VK_QUEUE_GRAPHICS_BIT) && supports_present)
            {
                graphics_index = k;
            }

            if (compute_index <= -1 && (family_property.queueFlags & VK_QUEUE_COMPUTE_BIT))
            {
                compute_index = k;
            }
        }

        if (graphics_index >= 0 && compute_index >= 0)
        {
            int8_t device_score = static_cast<int8_t>(device_properties.deviceType);
            if (device_score > device_type_score)
            {
                device_type_score = device_score;
                graphics_queue_index = graphics_index;
                compute_queue_index = compute_index;
                physical_device = physical_devices[i];
            }
        }
    }

    if (graphics_queue_index < 0 || compute_queue_index < 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "failed to find a suitable GPU");
        return false;
    }

    vulkan_renderer->graphics_queue_index = graphics_queue_index;
    vulkan_renderer->compute_queue_index = compute_queue_index;
    vulkan_renderer->physical_device = physical_device;

    uint32_t device_extension_count;
    VK_ASSERT(vkEnumerateDeviceExtensionProperties(vulkan_renderer->physical_device, nullptr, &device_extension_count, nullptr));

    std::vector<VkExtensionProperties> device_extensions(device_extension_count);
    VK_ASSERT(vkEnumerateDeviceExtensionProperties(vulkan_renderer->physical_device, nullptr, &device_extension_count, device_extensions.data()));

    VkPhysicalDeviceProperties physical_device_properties;
    vkGetPhysicalDeviceProperties(vulkan_renderer->physical_device, &physical_device_properties);
    vulkan_renderer->min_uniform_buffer_offset_alignment = physical_device_properties.limits.minUniformBufferOffsetAlignment;

    std::vector<const char*> enabled_extension_names;
    enabled_extension_names.push_back("VK_KHR_swapchain");

    for (uint32_t i = 0; i < enabled_extension_names.size(); ++i)
    {
        bool extension_supported_on_device = false;
        for (const VkExtensionProperties& extensionProperties : device_extensions)
        {
            if (strcmp(enabled_extension_names[i], extensionProperties.extensionName) == 0)
            {
                extension_supported_on_device = true;
                break;
            }
        }

        if (!extension_supported_on_device)
        {
            VkPhysicalDeviceProperties device_properties;
            vkGetPhysicalDeviceProperties(vulkan_renderer->physical_device, &device_properties);

            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "device %s does not support extension %s", device_properties.deviceName, enabled_extension_names[i]);;
            return false;
        }
    }

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;

    {
        VkDeviceQueueCreateInfo& device_queue_create_info = queue_create_infos.emplace_back();
        device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        device_queue_create_info.pNext = nullptr;
        device_queue_create_info.flags = 0;
        device_queue_create_info.queueCount = 1;
        float queue_priority = 1.0f;
        device_queue_create_info.pQueuePriorities = &queue_priority;
        device_queue_create_info.queueFamilyIndex = vulkan_renderer->graphics_queue_index;
    }

    if (vulkan_renderer->graphics_queue_index != vulkan_renderer->compute_queue_index)
    {
        VkDeviceQueueCreateInfo& device_queue_create_info = queue_create_infos.emplace_back();
        device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        device_queue_create_info.pNext = nullptr;
        device_queue_create_info.flags = 0;
        device_queue_create_info.queueCount = 1;
        float queue_priority = 1.0f;
        device_queue_create_info.pQueuePriorities = &queue_priority;
        device_queue_create_info.queueFamilyIndex = vulkan_renderer->compute_queue_index;
    }

    VkPhysicalDeviceFeatures physical_device_features;
    physical_device_features.robustBufferAccess = VK_FALSE;
    physical_device_features.fullDrawIndexUint32 = VK_FALSE;
    physical_device_features.imageCubeArray = VK_FALSE;
    physical_device_features.independentBlend = VK_FALSE;
    physical_device_features.geometryShader = VK_FALSE;
    physical_device_features.tessellationShader = VK_FALSE;
    physical_device_features.sampleRateShading = VK_FALSE;
    physical_device_features.dualSrcBlend = VK_FALSE;
    physical_device_features.logicOp = VK_FALSE;
    physical_device_features.multiDrawIndirect = VK_FALSE;
    physical_device_features.drawIndirectFirstInstance = VK_FALSE;
    physical_device_features.depthClamp = VK_FALSE;
    physical_device_features.depthBiasClamp = VK_FALSE;
    physical_device_features.fillModeNonSolid = VK_TRUE;
    physical_device_features.depthBounds = VK_FALSE;
    physical_device_features.wideLines = VK_FALSE;
    physical_device_features.largePoints = VK_FALSE;
    physical_device_features.alphaToOne = VK_FALSE;
    physical_device_features.multiViewport = VK_FALSE;
    physical_device_features.samplerAnisotropy = VK_TRUE;
    physical_device_features.textureCompressionETC2 = VK_FALSE;
    physical_device_features.textureCompressionASTC_LDR = VK_FALSE;
    physical_device_features.textureCompressionBC = VK_FALSE;
    physical_device_features.occlusionQueryPrecise = VK_FALSE;
    physical_device_features.pipelineStatisticsQuery = VK_FALSE;
    physical_device_features.vertexPipelineStoresAndAtomics = VK_FALSE;
    physical_device_features.fragmentStoresAndAtomics = VK_FALSE;
    physical_device_features.shaderTessellationAndGeometryPointSize = VK_FALSE;
    physical_device_features.shaderImageGatherExtended = VK_FALSE;
    physical_device_features.shaderStorageImageExtendedFormats = VK_FALSE;
    physical_device_features.shaderStorageImageMultisample = VK_FALSE;
    physical_device_features.shaderStorageImageReadWithoutFormat = VK_FALSE;
    physical_device_features.shaderStorageImageWriteWithoutFormat = VK_FALSE;
    physical_device_features.shaderUniformBufferArrayDynamicIndexing = VK_FALSE;
    physical_device_features.shaderSampledImageArrayDynamicIndexing = VK_FALSE;
    physical_device_features.shaderStorageBufferArrayDynamicIndexing = VK_FALSE;
    physical_device_features.shaderStorageImageArrayDynamicIndexing = VK_FALSE;
    physical_device_features.shaderClipDistance = VK_FALSE;
    physical_device_features.shaderCullDistance = VK_FALSE;
    physical_device_features.shaderFloat64 = VK_FALSE;
    physical_device_features.shaderInt64 = VK_FALSE;
    physical_device_features.shaderInt16 = VK_FALSE;
    physical_device_features.shaderResourceResidency = VK_FALSE;
    physical_device_features.shaderResourceMinLod = VK_FALSE;
    physical_device_features.sparseBinding = VK_FALSE;
    physical_device_features.sparseResidencyBuffer = VK_FALSE;
    physical_device_features.sparseResidencyImage2D = VK_FALSE;
    physical_device_features.sparseResidencyImage3D = VK_FALSE;
    physical_device_features.sparseResidency2Samples= VK_FALSE;
    physical_device_features.sparseResidency4Samples = VK_FALSE;
    physical_device_features.sparseResidency8Samples = VK_FALSE;
    physical_device_features.sparseResidency16Samples = VK_FALSE;
    physical_device_features.sparseResidencyAliased = VK_FALSE;
    physical_device_features.variableMultisampleRate = VK_FALSE;
    physical_device_features.inheritedQueries = VK_FALSE;

    VkDeviceCreateInfo device_create_info;
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext = nullptr;
    device_create_info.flags = 0;
    device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    device_create_info.pQueueCreateInfos = queue_create_infos.data();
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = nullptr;
    device_create_info.enabledExtensionCount = static_cast<uint32_t>(enabled_extension_names.size());
    device_create_info.ppEnabledExtensionNames = enabled_extension_names.data();
    device_create_info.pEnabledFeatures = &physical_device_features;

    VK_ASSERT(vkCreateDevice(vulkan_renderer->physical_device, &device_create_info, s_allocator, &vulkan_renderer->device));

    vkGetDeviceQueue(vulkan_renderer->device, vulkan_renderer->graphics_queue_index, 0, &vulkan_renderer->graphics_queue);
    if (vulkan_renderer->graphics_queue_index != vulkan_renderer->compute_queue_index)
    {
        vkGetDeviceQueue(vulkan_renderer->device, vulkan_renderer->compute_queue_index, 0, &vulkan_renderer->compute_queue);
    }

    return true;
}

bool vulkan_renderer_init_swapchain(GameWindow* game_window, VulkanRenderer* vulkan_renderer)
{
    uint32_t surface_format_count;
    VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan_renderer->physical_device, vulkan_renderer->window_surface, &surface_format_count, nullptr));

    SDL_assert(surface_format_count > 0);

    std::vector<VkSurfaceFormatKHR> surface_formats(surface_format_count);
    VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan_renderer->physical_device, vulkan_renderer->window_surface, &surface_format_count, surface_formats.data()));

    std::vector<VkFormat> preferred_surface_formats;
    preferred_surface_formats.push_back(VK_FORMAT_R8G8B8A8_SRGB);
    preferred_surface_formats.push_back(VK_FORMAT_B8G8R8A8_SRGB);
    preferred_surface_formats.push_back(VK_FORMAT_A8B8G8R8_SRGB_PACK32);

    VkSurfaceFormatKHR selected_surface_format;
    int32_t selected_surface_format_index = -1;

    for (VkSurfaceFormatKHR surface_format : surface_formats)
    {
        size_t loop_end_index = selected_surface_format_index >= 0 ? selected_surface_format_index : preferred_surface_formats.size();
        for (size_t i = 0; i < loop_end_index; ++i)// VkFormat preferred_format : preferred_surface_formats)
        {
            if (surface_format.format == preferred_surface_formats[i])
            {
                selected_surface_format_index = i;
                selected_surface_format = surface_format;
                break;
            }
        }
    }

    if (selected_surface_format_index < 0)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "no preferred swapchain format is supported by window surface, selecting first available one");
        selected_surface_format = surface_formats[0];
    }

    VkSurfaceCapabilitiesKHR surface_capabilities;
    VK_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan_renderer->physical_device, vulkan_renderer->window_surface, &surface_capabilities));

    VkExtent2D swapchain_size;
    if (surface_capabilities.currentExtent.width == 0xffffffff)
    {
        int window_width = 0;
        int window_height = 0;
        SDL_Vulkan_GetDrawableSize(game_window->window, &window_width, &window_height);

        swapchain_size.width = static_cast<uint32_t>(window_width);
        swapchain_size.height = static_cast<uint32_t>(window_height);
    }
    else
    {
        swapchain_size = surface_capabilities.currentExtent;
    }

    vulkan_renderer->swapchain_image_count = std::max(1u, surface_capabilities.minImageCount);

    VkSurfaceTransformFlagBitsKHR pre_transform;
    if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
    {
        pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else
    {
        pre_transform = surface_capabilities.currentTransform;
    }

    VkCompositeAlphaFlagBitsKHR composite_alpha_flag_bits = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if (surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
    {
        composite_alpha_flag_bits = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    }
    else if (surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
    {
        composite_alpha_flag_bits = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    }
    else if (surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
    {
        composite_alpha_flag_bits = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
    }
    else if (surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
    {
        composite_alpha_flag_bits = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
    }

    VkSwapchainKHR prev_swapchain = vulkan_renderer->swapchain;

    VkSwapchainCreateInfoKHR swapchain_create_info;
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.pNext = nullptr;
    swapchain_create_info.flags = 0;
    swapchain_create_info.surface = vulkan_renderer->window_surface;
    swapchain_create_info.minImageCount = vulkan_renderer->swapchain_image_count;
    swapchain_create_info.imageFormat = selected_surface_format.format;
    swapchain_create_info.imageColorSpace = selected_surface_format.colorSpace;
    swapchain_create_info.imageExtent.width = swapchain_size.width;
    swapchain_create_info.imageExtent.height = swapchain_size.height;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = 0;
    swapchain_create_info.pQueueFamilyIndices = nullptr;
    swapchain_create_info.preTransform = pre_transform;
    swapchain_create_info.compositeAlpha = composite_alpha_flag_bits;
    swapchain_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchain_create_info.clipped = true;
    swapchain_create_info.oldSwapchain = prev_swapchain;

    VK_ASSERT(vkCreateSwapchainKHR(vulkan_renderer->device, &swapchain_create_info, nullptr, &vulkan_renderer->swapchain));

    if (prev_swapchain != VK_NULL_HANDLE)
    {
        uint32_t swapchain_images_count;
        VK_ASSERT(vkGetSwapchainImagesKHR(vulkan_renderer->device, prev_swapchain, &swapchain_images_count, nullptr));

        for (uint32_t i = 0; i < swapchain_images_count; ++i)
        {
            vkDestroyImageView(vulkan_renderer->device, vulkan_renderer->swapchain_image_views[i], s_allocator);
        }

        delete[] vulkan_renderer->swapchain_image_views;

        vkDestroySwapchainKHR(vulkan_renderer->device, prev_swapchain, s_allocator);
    }

    vulkan_renderer->swapchain_width = swapchain_size.width;
    vulkan_renderer->swapchain_height = swapchain_size.height;
    vulkan_renderer->swapchain_format = selected_surface_format.format;

    VK_ASSERT(vkGetSwapchainImagesKHR(vulkan_renderer->device, vulkan_renderer->swapchain, &vulkan_renderer->swapchain_image_count, nullptr));

    std::vector<VkImage> swapchain_images(vulkan_renderer->swapchain_image_count);
    VK_ASSERT(vkGetSwapchainImagesKHR(vulkan_renderer->device, vulkan_renderer->swapchain, &vulkan_renderer->swapchain_image_count, swapchain_images.data()));

    vulkan_renderer->swapchain_image_views = new VkImageView[vulkan_renderer->swapchain_image_count];
    for (uint32_t i = 0; i < vulkan_renderer->swapchain_image_count; ++i)
    {
        vulkan_renderer->swapchain_image_views[i] = VK_NULL_HANDLE;
    }

    for (uint32_t i = 0; i < vulkan_renderer->swapchain_image_count; ++i)
    {
        VkImageViewCreateInfo image_view_create_info;
        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.pNext = nullptr;
        image_view_create_info.flags = 0;
        image_view_create_info.image = swapchain_images[i];
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format = vulkan_renderer->swapchain_format;
        image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_R;
        image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_G;
        image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_B;
        image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_A;
        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount = 1;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.layerCount = 1;

        VK_ASSERT(vkCreateImageView(vulkan_renderer->device, &image_view_create_info, nullptr, &vulkan_renderer->swapchain_image_views[i]));
    }

    return true;
}

bool vulkan_renderer_init_depth_stencil(VulkanRenderer* vulkan_renderer)
{
    VkImageCreateInfo image_create_info;
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = nullptr;
    image_create_info.flags = 0;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
    image_create_info.extent.width = vulkan_renderer->swapchain_width;
    image_create_info.extent.height = vulkan_renderer->swapchain_height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = 0;
    image_create_info.pQueueFamilyIndices = nullptr;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkResult result = vkCreateImage(vulkan_renderer->device, &image_create_info, s_allocator, &vulkan_renderer->depth_stencil_image);
    if (result != VK_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "failed to create depth stencil image");
        return false;
    }

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(vulkan_renderer->device, vulkan_renderer->depth_stencil_image, &memory_requirements);

    VkMemoryAllocateInfo memory_allocate_info;
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.pNext = nullptr;
    memory_allocate_info.allocationSize = memory_requirements.size;
    if (!vulkan_renderer_find_memory_by_flag_and_type(vulkan_renderer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memory_requirements.memoryTypeBits, &memory_allocate_info.memoryTypeIndex))
    {
        return false;
    }

    result = vkAllocateMemory(vulkan_renderer->device, &memory_allocate_info, s_allocator, &vulkan_renderer->depth_stencil_image_memory);
    if (result != VK_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "failed to allocate memory for depth stencil image");
        return false;
    }

    result = vkBindImageMemory(vulkan_renderer->device, vulkan_renderer->depth_stencil_image, vulkan_renderer->depth_stencil_image_memory, 0);
    if (result != VK_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "failed to bind memory to image for depth stencil image");
        return false;
    }

    VkImageViewCreateInfo image_view_create_info;
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext = nullptr;
    image_view_create_info.flags = 0;
    image_view_create_info.image = vulkan_renderer->depth_stencil_image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = image_create_info.format;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;

    result = vkCreateImageView(vulkan_renderer->device, &image_view_create_info, s_allocator, &vulkan_renderer->depth_stencil_image_view);
    if (result != VK_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "failed to create image view for depth stencil image view");
        return false;
    }

    return true;
}

VulkanRenderer* vulkan_renderer_init(struct GameWindow* game_window)
{
    VulkanRenderer* vulkan_renderer = new VulkanRenderer();
    bool init_failed = false;

    if (!init_failed && !vulkan_renderer_init_instance(game_window, vulkan_renderer))
    {
        init_failed = true;
    }

    if (!init_failed && !vulkan_renderer_init_device(vulkan_renderer))
    {
        init_failed = true;
    }

    if (!init_failed && !vulkan_renderer_init_swapchain(game_window, vulkan_renderer))
    {
        init_failed = true;
    }

    if (!init_failed && !vulkan_renderer_init_depth_stencil(vulkan_renderer))
    {
        init_failed = true;
    }

    if (init_failed)
    {
        vulkan_renderer_destroy(vulkan_renderer);
        return nullptr;
    }

    return vulkan_renderer;
}

void vulkan_renderer_destroy_instance(VulkanRenderer* vulkan_renderer)
{
#ifdef VK_DEBUG
    if (vulkan_renderer->debug_report_callback_ext)
    {
        PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(vulkan_renderer->instance, "vkDestroyDebugReportCallbackEXT");
        SDL_assert(vkDestroyDebugReportCallbackEXT);

        vkDestroyDebugReportCallbackEXT(vulkan_renderer->instance, vulkan_renderer->debug_report_callback_ext, s_allocator);
    }
#endif // VK_DEBUG

    if (vulkan_renderer->window_surface)
    {
        vkDestroySurfaceKHR(vulkan_renderer->instance, vulkan_renderer->window_surface, s_allocator);
    }

    if (vulkan_renderer->instance)
    {
        vkDestroyInstance(vulkan_renderer->instance, s_allocator);
    }
}

void vulkan_renderer_destroy_device(VulkanRenderer* vulkan_renderer)
{
    if (vulkan_renderer->device)
    {
        vkDestroyDevice(vulkan_renderer->device, s_allocator);
    }
}

void vulkan_renderer_destroy_swapchain(VulkanRenderer* vulkan_renderer)
{
    if (vulkan_renderer->swapchain_image_views)
    {
        for (uint32_t i = 0; i < vulkan_renderer->swapchain_image_count; ++i)
        {
            VkImageView image_view = vulkan_renderer->swapchain_image_views[i];
            if (image_view)
            {
                vkDestroyImageView(vulkan_renderer->device, image_view, s_allocator);
            }
        }

        delete[] vulkan_renderer->swapchain_image_views;
    }

    if (vulkan_renderer->swapchain)
    {
        vkDestroySwapchainKHR(vulkan_renderer->device, vulkan_renderer->swapchain, s_allocator);
    }
}

void vulkan_renderer_destroy_depth_stencil(VulkanRenderer* vulkan_renderer)
{
    if (vulkan_renderer->depth_stencil_image_view)
    {
        vkDestroyImageView(vulkan_renderer->device, vulkan_renderer->depth_stencil_image_view, s_allocator);
    }

    if (vulkan_renderer->depth_stencil_image)
    {
        vkDestroyImage(vulkan_renderer->device, vulkan_renderer->depth_stencil_image, s_allocator);
    }

    if (vulkan_renderer->depth_stencil_image_memory)
    {
        vkFreeMemory(vulkan_renderer->device, vulkan_renderer->depth_stencil_image_memory, s_allocator);
    }
}

void vulkan_renderer_destroy(VulkanRenderer* vulkan_renderer)
{
    SDL_assert(vulkan_renderer);

    //vkDeviceWaitIdle(m_vulkanDevice);

    vulkan_renderer_destroy_depth_stencil(vulkan_renderer);
    vulkan_renderer_destroy_swapchain(vulkan_renderer);
    vulkan_renderer_destroy_device(vulkan_renderer);
    vulkan_renderer_destroy_instance(vulkan_renderer);

    delete vulkan_renderer;
}
