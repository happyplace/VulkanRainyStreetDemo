#include "GameWindow.h"

int main(int argc, char** argv)
{
    GameWindow* window = init_game_window();
    if (window == nullptr)
    {
        return 1;
    }
    destory_game_window(window);
    return 0;
}

