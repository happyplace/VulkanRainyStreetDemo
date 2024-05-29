#include "GameWindow.h"

#include <SDL.h>

struct GameWindow
{
    SDL_Window* window;
};

GameWindow* init_game_window()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0)
    {
        return nullptr;
    }

    GameWindow* game_window = new GameWindow();
    return game_window;
}

void destory_game_window(GameWindow* game_window)
{
    SDL_assert(game_window);

    delete game_window;
}
