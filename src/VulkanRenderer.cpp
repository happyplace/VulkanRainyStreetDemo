#include "VulkanRenderer.h"

#include <inttypes.h>
#include <vector>

#include <SDL_assert.h>
#include <SDL_log.h>
#include <SDL_vulkan.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "GameWindow.h"

#define VK_ASSERT(X) SDL_assert(VK_SUCCESS == X)
#define VK_DEBUG
#define VK_PORTABILITY

static VkAllocationCallbacks* s_allocator = nullptr;

struct VulkanRenderer
{
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR window_surface = VK_NULL_HANDLE;

#ifdef VK_DEBUG
    VkDebugReportCallbackEXT debug_report_callback_ext = VK_NULL_HANDLE;
#endif // VK_DEBUG
};

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

VulkanRenderer* vulkan_renderer_init(struct GameWindow* game_window)
{
    VulkanRenderer* vulkan_renderer = new VulkanRenderer();

    if (!vulkan_renderer_init_instance(game_window, vulkan_renderer))
    {
        return nullptr;
    }

    return vulkan_renderer;
}

void vulkan_renderer_destory(VulkanRenderer* vulkan_renderer)
{
    SDL_assert(vulkan_renderer);

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

    delete vulkan_renderer;
}
