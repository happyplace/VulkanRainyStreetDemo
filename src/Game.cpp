#include "Game.h"

#include "SDL_assert.h"

#include "GameWindow.h"
#include "VulkanRenderer.h"
#include "VulkanFrameResources.h"
#include "GameMap.h"
#include "GameFrameRender.h"

#ifdef IMGUI_ENABLED
#include "ImGuiRenderer.h"
#endif // IMGUI_ENABLED

void game_destroy(Game* game)
{
    SDL_assert(game);

#ifdef IMGUI_ENABLED
    if (game->imgui_renderer)
    {
        imgui_renderer_destroy(game, game->imgui_renderer);
    }
#endif // IMGUI_ENABLED

    if (game->frame_resources)
    {
        vulkan_frame_resources_destroy(game->vulkan_renderer, game->frame_resources);
    }

    if (game->vulkan_renderer)
    {
       vulkan_renderer_destroy(game->vulkan_renderer);
    }

    if (game->game_window)
    {
       game_window_destory(game->game_window);
    }

    if (game->game_map)
    {
        game_map_destroy(game->game_map);
    }

    delete game;
}

Game* game_init()
{
    Game* game = new Game();

    game->game_window = game_window_init();
    if (game->game_window == nullptr)
    {
        game_destroy(game);
        return nullptr;
    }

    game->vulkan_renderer = vulkan_renderer_init(game->game_window);
    if (game->vulkan_renderer == nullptr)
    {
        game_destroy(game);
        return nullptr;
    }

    game->frame_resources = vulkan_frame_resources_init(game->vulkan_renderer);
    if (game->frame_resources == nullptr)
    {
        game_destroy(game);
        return nullptr;
    }

    game->game_map = game_map_init();
    if (game->game_map == nullptr)
    {
        game_destroy(game);
        return nullptr;
    }

#ifdef IMGUI_ENABLED
    game->imgui_renderer = imgui_renderer_init(game);
    if (game->imgui_renderer == nullptr)
    {
        game_destroy(game);
        return nullptr;
    }
#endif // IMGUI_ENABLED

    return game;
}

void game_on_window_resized(Game* game)
{
    game->game_window->window_resized = false;
    vulkan_renderer_on_window_resized(game->game_window, game->vulkan_renderer);
#ifdef IMGUI_ENABLED
    imgui_renderer_on_resize(game->vulkan_renderer, game->imgui_renderer);
#endif // IMGUI_ENABLED
}

int game_run(int argc, char** argv)
{
    Game* game = game_init();
    if (game == nullptr)
    {
        return 1;
    }

    while (!game->game_window->quit_requested)
    {
        game_window_process_events(game->game_window);

        if (game->game_window->window_resized)
        {
            game_on_window_resized(game);
        }

        FrameResource* frame_resource = game_frame_render_begin_frame(game);
        game_frame_render_end_frame(game, frame_resource);

#ifdef IMGUI_ENABLED
        imgui_renderer_draw(game->vulkan_renderer, frame_resource, game->imgui_renderer);
#endif // IMGUI_ENABLED

        game_frame_render_submit(game, frame_resource);
    }

    game_destroy(game);
    return 0;
}
