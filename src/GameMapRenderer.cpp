#include "GameMapRenderer.h"

#include <SDL_assert.h>

struct GameMapRenderer
{

};

void game_map_renderer_destroy(GameMapRenderer* game_map_renderer)
{
    SDL_assert(game_map_renderer);

    delete game_map_renderer;
}

GameMapRenderer* game_map_renderer_init()
{
    GameMapRenderer* game_map_renderer = new GameMapRenderer();

    return game_map_renderer;
}
