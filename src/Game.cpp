#include "Game.h"

#include "GameTimer.h"
#include "SDL_assert.h"

#include "imgui.h"

#include "GameWindow.h"
#include "VulkanRenderer.h"
#include "VulkanFrameResources.h"
#include "GameMap.h"
#include "GameFrameRender.h"
#include "ImGuiRenderer.h"

void game_destroy(Game* game)
{
    SDL_assert(game);

    if (game->game_timer)
    {
        game_timer_destroy(game->game_timer);
    }

#ifdef IMGUI_ENABLED
    if (game->imgui_renderer)
    {
        imgui_renderer_destroy(game->imgui_renderer, game);
    }
#endif // IMGUI_ENABLED

    if (game->frame_resources)
    {
        vulkan_frame_resources_destroy(game->frame_resources, game->vulkan_renderer);
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

    game->game_timer = game_timer_init();
    if (game->game_timer == nullptr)
    {
        game_destroy(game);
        return nullptr;
    }

    return game;
}

void game_on_window_resized(Game* game)
{
    game->game_window->window_resized = false;
    vulkan_renderer_on_window_resized(game->vulkan_renderer, game->game_window);
#ifdef IMGUI_ENABLED
    imgui_renderer_on_resize(game->imgui_renderer, game->vulkan_renderer);
#endif // IMGUI_ENABLED
}

int game_run(int argc, char** argv)
{
    Game* game = game_init();
    if (game == nullptr)
    {
        return 1;
    }

    game_timer_reset(game->game_timer);

    while (!game->game_window->quit_requested)
    {
        game_window_process_events(game->game_window);

        if (game->game_window->window_resized)
        {
            game_on_window_resized(game);
        }

        game_timer_tick(game->game_timer);

        FrameResource* frame_resource = game_frame_render_begin_frame(game);
        game_frame_render_end_frame(frame_resource, game);

#ifdef IMGUI_ENABLED
        imgui_renderer_draw(game->imgui_renderer, game, frame_resource);
#endif // IMGUI_ENABLED

        game_frame_render_submit(frame_resource, game);
    }

    game_destroy(game);
    return 0;
}

void game_imgui_stats_window()
{
    bool is_opened = true;
    if (ImGui::Begin("Game Stats", &is_opened, ImGuiWindowFlags_None))
    {
        ImGui::Text("andrew was here");
    }
    ImGui::End();
}
