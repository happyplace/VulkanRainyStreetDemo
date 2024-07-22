#include "GameMapRenderer.h"

#include <SDL_assert.h>

#include "PhongMeshRenderer.h"
#include "Game.h"

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

void game_map_renderer_render(GameMapRenderer* game_map_renderer, struct FrameResource* frame_resource, struct Game* game)
{
    phong_mesh_renderer_render(game->mesh_renderer, frame_resource, game);
}
