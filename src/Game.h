#ifndef VRSD_Game_h_
#define VRSD_Game_h_

struct GameWindow;
struct VulkanRenderer;
struct VulkanFrameResources;
struct GameMap;
struct ImGuiRenderer;
struct GameTimer;
struct MeshRenderer;
struct VulkanSharedResources;

struct Game
{
    GameWindow* game_window = nullptr;
    VulkanRenderer* vulkan_renderer = nullptr;
    VulkanFrameResources* frame_resources = nullptr;
    VulkanSharedResources* shared_resources = nullptr;
    GameMap* game_map = nullptr;
    ImGuiRenderer* imgui_renderer = nullptr;
    GameTimer* game_timer = nullptr;
    MeshRenderer* mesh_renderer = nullptr;
};

int game_run(int argc, char** argv);

void game_abort();

void game_imgui_stats_window(struct FrameResource* frame_resource);

#endif // VRSD_Game_h_
