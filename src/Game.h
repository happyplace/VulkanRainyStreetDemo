#ifndef VRSD_Game_h_
#define VRSD_Game_h_

struct GameWindow;
struct VulkanRenderer;
struct VulkanFrameResources;
struct GameMap;
struct ImGuiRenderer;
struct GameTimer;
struct PhongMeshRenderer;
struct VulkanSharedResources;

namespace ftl { class TaskScheduler; }

struct Game
{
    GameWindow* game_window = nullptr;
    VulkanRenderer* vulkan_renderer = nullptr;
    VulkanFrameResources* frame_resources = nullptr;
    VulkanSharedResources* shared_resources = nullptr;
    ImGuiRenderer* imgui_renderer = nullptr;
    GameTimer* game_timer = nullptr;
    PhongMeshRenderer* mesh_renderer = nullptr;
    GameMap* game_map = nullptr;
};

struct GameInitParams
{
    int argc;
    char** argv;
    Game* game = nullptr;
    GameWindow* game_window = nullptr;
};

void game_init_task(ftl::TaskScheduler* task_scheduler, void* arg);
void game_destroy(Game* game);

void game_abort();

void game_imgui_stats_window(struct FrameResource* frame_resource);

#endif // VRSD_Game_h_
