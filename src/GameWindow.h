#ifndef VRSD_GameWindow_h_
#define VRSD_GameWindow_h_

struct SDL_Window;

struct GameWindow
{
    SDL_Window* window = nullptr;
    bool quit_requested = false;
    bool window_resized = false;
};

GameWindow* game_window_init();
void game_window_destory(GameWindow* game_window);

void game_window_process_events(GameWindow* game_window);

#endif // VRSD_GameWindow_h_
