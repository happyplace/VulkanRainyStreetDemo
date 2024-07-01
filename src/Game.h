#ifndef VRSD_Game_h_
#define VRSD_Game_h_

struct GameWindow;
struct VulkanRenderer;
struct VulkanFrameResources;
struct GameMap;

struct Game
{
    GameWindow* game_window = nullptr;
    VulkanRenderer* vulkan_renderer = nullptr;
    VulkanFrameResources* frame_resources = nullptr;
    GameMap* game_map = nullptr;
};

int game_run(int argc, char** argv);

#endif // VRSD_Game_h_
