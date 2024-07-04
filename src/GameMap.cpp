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

GameMap* game_map_init()
{
    GameMap* game_map = new GameMap();

    game_map->map_renderer = game_map_renderer_init();
    if (game_map->map_renderer == nullptr)
    {
        game_map_destroy(game_map);
        return nullptr;
    }

    return game_map;
}

void game_map_load(GameMap* game_map)
{

}

void game_map_update(struct Game* game, struct FrameResource* frame_resource)
{

}

void game_map_render(struct Game* game, struct FrameResource* frame_resource)
{

}
