#ifndef VRSD_Game_h_
#define VRSD_Game_h_

struct GameWindow;
struct VulkanRenderer;
struct VulkanFrameResources;
struct GameMap;
struct ImGuiRenderer;

struct Game
{
    GameWindow* game_window = nullptr;
    VulkanRenderer* vulkan_renderer = nullptr;
    VulkanFrameResources* frame_resources = nullptr;
    GameMap* game_map = nullptr;
    ImGuiRenderer* imgui_renderer = nullptr;
};

int game_run(int argc, char** argv);

void game_on_window_resized(Game* game);

#endif // VRSD_Game_h_
