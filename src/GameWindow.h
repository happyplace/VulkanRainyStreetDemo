#ifndef VRSD_GameWindow_h_
#define VRSD_GameWindow_h_

#include <cstdint>

struct SDL_Window;

enum class GameWindowFlag : uint64_t
{
    QuitRequested = 1 << 0,
    ResizeRequested = 1 << 1,
};

struct GameWindow
{
    SDL_Window* window = nullptr;
    uint64_t window_flags = 0;
};

GameWindow* game_window_init();
void game_window_destroy(GameWindow* game_window);

void game_window_process_events(GameWindow* game_window);

void game_window_set_window_flag(GameWindow* game_window, GameWindowFlag flag, bool value);
bool game_window_get_window_flag(GameWindow* game_window, GameWindowFlag flag);

#endif // VRSD_GameWindow_h_
