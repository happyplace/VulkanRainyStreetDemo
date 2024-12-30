#include "Game.h"

#include "GameTimer.h"
#include "SDL_assert.h"

#include "VulkanSharedResources.h"
#include "imgui.h"

#include "GameWindow.h"
#include "VulkanRenderer.h"
#include "VulkanFrameResources.h"
#include "GameMap.h"
#include "GameFrameRender.h"
#include "ImGuiRenderer.h"
#include "PhongMeshRenderer.h"

void game_init_task(ftl::TaskScheduler* task_scheduler, void* arg)
{
    GameInitParams* params = reinterpret_cast<GameInitParams*>(arg);
}

void game_destroy(Game* game)
{
    SDL_assert(game);

    if (game->mesh_renderer)
    {
        phong_mesh_renderer_destroy(game->mesh_renderer, game);
    }

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

    if (game->shared_resources)
    {
        vulkan_shared_resources_destroy(game->shared_resources, game->vulkan_renderer);
    }

    if (game->frame_resources)
    {
        vulkan_frame_resources_destroy(game->frame_resources, game->vulkan_renderer);
    }

    if (game->vulkan_renderer)
    {
       vulkan_renderer_destroy(game->vulkan_renderer);
    }

    delete game;
}

Game* game_init()
{
    Game* game = new Game();

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

    game->shared_resources = vulkan_shared_resources_init(game->vulkan_renderer);
    if (game->shared_resources == nullptr)
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

    game->mesh_renderer = phong_mesh_renderer_init(game);
    if (game->mesh_renderer == nullptr)
    {
        game_destroy(game);
        return nullptr;
    }

    return game;
}

void game_resize_if_required(Game* game)
{
    if (!game_window_get_window_flag(game->game_window, GameWindowFlag::ResizeRequested))
    {
        return;
    }

    game_window_set_window_flag(game->game_window, GameWindowFlag::ResizeRequested, false);

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

    GameMap* game_map = game_map_init();
    if (game_map == nullptr)
    {
        return 1;
    }

    game_map_load(game_map);

    game_timer_reset(game->game_timer);

    while (!game_window_get_window_flag(game->game_window, GameWindowFlag::QuitRequested))
    {
        game_window_process_events(game->game_window);
        game_resize_if_required(game);

        game_timer_tick(game->game_timer);

        FrameResource* frame_resource = game_frame_render_begin_frame(game);

        if (frame_resource == nullptr)
        {
            continue;
        }

        frame_resource->time.delta_time = game_timer_delta_time(game->game_timer);
        frame_resource->time.total_time = game_timer_total_time(game->game_timer);
        frame_resource->time.frame_number = game_timer_frame_count(game->game_timer);

        game_map_update(game_map, frame_resource, game);
        game_map_render(game_map, frame_resource, game);

        game_frame_render_end_frame(frame_resource, game);

#ifdef IMGUI_ENABLED
        imgui_renderer_draw(game->imgui_renderer, game, frame_resource);
#endif // IMGUI_ENABLED

        game_frame_render_submit(frame_resource, game);
    }

    game_map_destroy(game_map);
    game_destroy(game);
    return 0;
}

void game_imgui_stats_window(struct FrameResource* frame_resource)
{
    constexpr double time_span_duration = 1.0;

    static double elapsed_time = 0.0;
    static uint32_t elapsed_frame_count = 0;

    static uint32_t frame_count = 0;

    elapsed_frame_count++;
    elapsed_time += frame_resource->time.delta_time;
    if (elapsed_time >= time_span_duration)
    {
        frame_count = elapsed_frame_count;

        elapsed_time -= time_span_duration;
        elapsed_frame_count = 0;
    }

    if (ImGui::Begin("Game Stats", nullptr, ImGuiWindowFlags_None))
    {
        ImGui::Text("FPS: %u", frame_count);
    }
    ImGui::End();
}

void game_abort()
{
    SDL_assert(false);
}
