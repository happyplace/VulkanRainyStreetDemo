#ifndef VRSD_VulkanRenderer_h_
#define VRSD_VulkanRenderer_h_

#include <vulkan/vulkan_core.h>

#define VK_DEBUG
//#define VK_PORTABILITY

struct VulkanRenderer
{
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR window_surface = VK_NULL_HANDLE;
#ifdef VK_DEBUG
    VkDebugReportCallbackEXT debug_report_callback_ext = VK_NULL_HANDLE;
#endif // VK_DEBUG
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    uint32_t graphics_queue_index = 0;
    uint32_t compute_queue_index = 0;
    VkDeviceSize min_uniform_buffer_offset_alignment = 0;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphics_queue = VK_NULL_HANDLE;
    VkQueue compute_queue = VK_NULL_HANDLE;
    uint32_t swapchain_image_count = 0;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkImageView* swapchain_image_views = nullptr;
    uint32_t swapchain_width = 0;
    uint32_t swapchain_height = 0;
    VkFormat swapchain_format = VK_FORMAT_UNDEFINED;
};

VulkanRenderer* vulkan_renderer_init(struct GameWindow* game_window);
void vulkan_renderer_destroy(VulkanRenderer* vulkan_renderer);

#endif // VRSD_VulkanRenderer_h_
