#include "GameWindow.h"
#include "SDL_video.h"

#include <SDL.h>
#include <SDL_log.h>
#include <SDL_assert.h>

#ifdef IMGUI_ENABLED
#include "ImGuiRenderer.h"
#endif // IMGUI_ENABLED

GameWindow* game_window_init()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "error initing SDL: %s", SDL_GetError());
        return nullptr;
    }

    GameWindow* game_window = new GameWindow();

    game_window->window = SDL_CreateWindow(
        "Vulkan Rainy Street Demo",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        1280, 720,
        SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (game_window->window == nullptr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "failed to create SDL window: %s", SDL_GetError());
        return nullptr;
    }

    return game_window;
}

void game_window_destory(GameWindow* game_window)
{
    SDL_assert(game_window);

    if (game_window->window != nullptr)
    {
        SDL_DestroyWindow(game_window->window);
    }

    delete game_window;

    SDL_Quit();
}

void game_window_process_events(GameWindow* game_window)
{
    SDL_Event sdl_event;
    while (SDL_PollEvent(&sdl_event) != 0)
    {
#ifdef IMGUI_ENABLED
        // TODO: this is a weird dependency, we need a way to register, set a priority, and consume events
        if (imgui_renderer_on_sdl_event(&sdl_event))
        {
            continue;
        }
#endif // IMGUI_ENABLED

        if (sdl_event.type == SDL_QUIT)
        {
            game_window->quit_requested = true;
        }
        else if (sdl_event.type == SDL_WINDOWEVENT_RESIZED)
        {
            if (sdl_event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                game_window->window_resized = true;
            }
        }
    }
}
