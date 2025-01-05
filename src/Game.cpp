#include "Game.h"

#include <array>

#include "ftl/task_scheduler.h"

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

static Game* s_game = nullptr;

Game* game_get_instance()
{
    return s_game;
}

void game_destroy(Game* game)
{
    SDL_assert(game);
    s_game = nullptr;

    if (game->game_map)
    {
        game_map_destroy(game->game_map);
    }

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

void game_frame_task(ftl::TaskScheduler* task_scheduler, void* arg)
{
    Game* game = reinterpret_cast<Game*>(arg);

    if (game_window_get_window_flag(game->game_window, GameWindowFlag::QuitRequested))
    {
        return;
    }

    game_resize_if_required(game);

    game_timer_tick(game->game_timer);

    FrameResource* frame_resource = game_frame_render_begin_frame(game);

    if (frame_resource == nullptr)
    {
        task_scheduler->AddTask({ game_frame_task, game }, ftl::TaskPriority::High);
        return;
    }

    frame_resource->time.delta_time = game_timer_delta_time(game->game_timer);
    frame_resource->time.total_time = game_timer_total_time(game->game_timer);
    frame_resource->time.frame_number = game_timer_frame_count(game->game_timer);

    GameMapUpdateParams game_map_update_params;
    game_map_update_params.game = game;
    game_map_update_params.game_map = game->game_map;
    game_map_update_params.frame_resource = frame_resource;

    ftl::WaitGroup wait_group(task_scheduler);
    task_scheduler->AddTask({ game_map_update_task, &game_map_update_params }, ftl::TaskPriority::High, &wait_group);
    wait_group.Wait();

    game_map_render(game->game_map, frame_resource, game);

    game_frame_render_end_frame(frame_resource, game);

#ifdef IMGUI_ENABLED
    imgui_renderer_draw(game->imgui_renderer, game, frame_resource);
#endif // IMGUI_ENABLED

    game_frame_render_submit(frame_resource, game);

    task_scheduler->AddTask({ game_frame_task, game }, ftl::TaskPriority::High);
}

void game_init_task(ftl::TaskScheduler* task_scheduler, void* arg)
{
    GameInitParams* params = reinterpret_cast<GameInitParams*>(arg);

    s_game = new Game();
    Game* game = s_game;
    params->game = game;

    game->game_window = params->game_window;

    game->vulkan_renderer = vulkan_renderer_init(game->game_window);
    if (game->vulkan_renderer == nullptr)
    {
        game_window_set_window_flag(game->game_window, GameWindowFlag::QuitRequested, true);
        return;
    }

    game->frame_resources = vulkan_frame_resources_init(game->vulkan_renderer);
    if (game->frame_resources == nullptr)
    {
        game_window_set_window_flag(game->game_window, GameWindowFlag::QuitRequested, true);
        return;
    }

#ifdef IMGUI_ENABLED
    game->imgui_renderer = imgui_renderer_init(game);
    if (game->imgui_renderer == nullptr)
    {
        game_window_set_window_flag(game->game_window, GameWindowFlag::QuitRequested, true);
        return;
    }
#endif // IMGUI_ENABLED

    game->game_timer = game_timer_init();
    if (game->game_timer == nullptr)
    {
        game_window_set_window_flag(game->game_window, GameWindowFlag::QuitRequested, true);
        return;
    }

    constexpr uint32_t parallel_init_task_count = 2;
    ftl::Task parallel_init_tasks[parallel_init_task_count];

    GameMapInitParams game_map_init_params;
    game_map_init_params.game = game;
    game_map_init_params.game_map = nullptr;
    parallel_init_tasks[0] = { game_map_init_and_load_task, &game_map_init_params };

    VulkanSharedResourcesInitParams vulkan_shared_resources_init_params;
    vulkan_shared_resources_init_params.vulkan_renderer = game->vulkan_renderer;
    vulkan_shared_resources_init_params.vulkan_shared_resources = nullptr;
    vulkan_shared_resources_init_params.init_successful = false;
    parallel_init_tasks[1] = { vulkan_shared_resources_init_task, &vulkan_shared_resources_init_params };

    ftl::WaitGroup wait_group(task_scheduler);
    task_scheduler->AddTasks(parallel_init_task_count, parallel_init_tasks, ftl::TaskPriority::High, &wait_group);
    wait_group.Wait();

    game->game_map = game_map_init_params.game_map;
    game->shared_resources = vulkan_shared_resources_init_params.vulkan_shared_resources;
    if (game->game_map == nullptr || game->shared_resources == nullptr)
    {
        game_window_set_window_flag(game->game_window, GameWindowFlag::QuitRequested, true);
        return;
    }

    if (!vulkan_shared_resources_init_params.init_successful)
    {
        game_window_set_window_flag(game->game_window, GameWindowFlag::QuitRequested, true);
        return;
    }

    game->mesh_renderer = phong_mesh_renderer_init(game);
    if (game->mesh_renderer == nullptr)
    {
        game_window_set_window_flag(game->game_window, GameWindowFlag::QuitRequested, true);
        return;
    }

    game_timer_reset(game->game_timer);

    task_scheduler->AddTask({ game_frame_task, game }, ftl::TaskPriority::High);
}
