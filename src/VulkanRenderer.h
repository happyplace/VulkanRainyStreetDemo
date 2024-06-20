#ifndef VRSD_VulkanRenderer_h_
#define VRSD_VulkanRenderer_h_

struct VulkanRenderer;

VulkanRenderer* vulkan_renderer_init(struct GameWindow* game_window);
void vulkan_renderer_destory(VulkanRenderer* vulkan_renderer);

#endif // VRSD_VulkanRenderer_h_
