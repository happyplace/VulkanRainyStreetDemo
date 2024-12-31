#include "GameMap.h"

#include <SDL_assert.h>

#include "GameMapRenderer.h"

void game_map_destroy(GameMap* game_map)
{
    SDL_assert(game_map);

    if (game_map->map_renderer)
    {
        game_map_renderer_destroy(game_map->map_renderer);
    }

    delete game_map;
}

void game_map_init_and_load_task(ftl::TaskScheduler* /*task_scheduler*/, void* arg)
{
    GameMapInitParams* params = reinterpret_cast<GameMapInitParams*>(arg);
    
    params->game_map = new GameMap();
    
    params->game_map->map_renderer = game_map_renderer_init();
    if (params->game_map->map_renderer == nullptr)
    {
        game_map_destroy(params->game_map);
        params->game_map = nullptr;
        return;
    }
}

void game_map_imgui_draw()
{
    game_map_renderer_imgui_draw();
}

void game_map_update_task(ftl::TaskScheduler* /*task_scheduler*/, void* /*arg*/)
{
    //GameMapUpdateParams* params = reinterpret_cast<GameMapUpdateParams*>(arg);
}

void game_map_render(GameMap* game_map, struct FrameResource* frame_resource, struct Game* game)
{
    game_map_renderer_render(game_map->map_renderer, frame_resource, game);
}
