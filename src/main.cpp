#include "GameWindow.h"
#include "VulkanRenderer.h"

int main(int argc, char** argv)
{
    GameWindow* window = game_window_init();
    if (window == nullptr)
    {
        return 1;
    }

    VulkanRenderer* renderer = vulkan_renderer_init(window);
    if (renderer == nullptr)
    {
        return 1;
    }

    while (!game_window_is_quit_requested(window))
    {
        game_window_process_events(window);
    }

    vulkan_renderer_destory(renderer);
    game_window_destory(window);
    return 0;
}
