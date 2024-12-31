#ifndef VRSD_GameMap_h_
#define VRSD_GameMap_h_

struct GameMapRenderer;
struct Game;
struct FrameResource;

namespace ftl { class TaskScheduler; }

struct GameMap
{
    GameMapRenderer* map_renderer = nullptr;
};

struct GameMapInitParams
{
    GameMap* game_map;
    Game* game;
};

struct GameMapUpdateParams
{
    Game* game = nullptr;
    GameMap* game_map = nullptr;
    FrameResource* frame_resource = nullptr;
};

void game_map_init_and_load_task(ftl::TaskScheduler* task_scheduler, void* arg);
void game_map_update_task(ftl::TaskScheduler* task_scheduler, void* arg);

void game_map_destroy(GameMap* game_map);

void game_map_render(GameMap* game_map, struct FrameResource* frame_resource, struct Game* game);
void game_map_imgui_draw();

#endif // VRSD_GameMap_h_
