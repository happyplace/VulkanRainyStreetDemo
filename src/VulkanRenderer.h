#ifndef VRSD_VulkanRenderer_h_
#define VRSD_VulkanRenderer_h_

#include <cstddef>
#include <cstdint>

#include <vulkan/vulkan_core.h>

#include "shaderc/shaderc.h"

struct VkAllocationCallbacks;
VK_DEFINE_HANDLE(VmaAllocator);

static VkAllocationCallbacks* s_allocator = nullptr;

#define VK_ASSERT(X) SDL_assert(VK_SUCCESS == X)

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
    VkImage depth_stencil_image = VK_NULL_HANDLE;
    VkDeviceMemory depth_stencil_image_memory = VK_NULL_HANDLE;
    VkImageView depth_stencil_image_view = VK_NULL_HANDLE;
    VkRenderPass render_pass = VK_NULL_HANDLE;
    VkFramebuffer* framebuffers = nullptr;
    VmaAllocator vma_allocator = VK_NULL_HANDLE;
    shaderc_compiler_t shaderc_compiler = nullptr;
    VkCommandPool transfer_command_pool = VK_NULL_HANDLE;
    VkCommandBuffer transfer_command_buffer = VK_NULL_HANDLE;
    VkFence transfer_fence = VK_NULL_HANDLE;
};

VulkanRenderer* vulkan_renderer_init(struct GameWindow* game_window);
void vulkan_renderer_destroy(VulkanRenderer* vulkan_renderer);

void vulkan_renderer_on_window_resized(VulkanRenderer* vulkan_renderer, struct GameWindow* game_window);

bool vulkan_renderer_different_compute_and_graphics_queue(VulkanRenderer* vulkan_renderer, struct GameWindow* game_window);
void vulkan_renderer_wait_device_idle(VulkanRenderer* vulkan_renderer);
VkDeviceSize vulkan_renderer_calculate_uniform_buffer_size(VulkanRenderer* vulkan_renderer, size_t size);
bool vulkan_renderer_compile_shader(VulkanRenderer* vulkan_renderer, const char* path, shaderc_compile_options_t compile_options, shaderc_shader_kind shader_kind, VkShaderModule* out_shader_module);

#endif // VRSD_VulkanRenderer_h_
