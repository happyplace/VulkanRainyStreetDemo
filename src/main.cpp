#include "ftl/task_scheduler.h"
#include "ftl/wait_group.h"

#include "Game.h"
#include "GameWindow.h"

int main(int argc, char** argv)
{
    GameWindow* game_window = game_window_init();
    if (game_window == nullptr)
    {
        return 1;
    }

    ftl::TaskScheduler* task_scheduler = new ftl::TaskScheduler();
    task_scheduler->Init();

    GameInitParams game_init_params;
    game_init_params.argc = argc;
    game_init_params.argv = argv;
    game_init_params.game = nullptr;
    ftl::Task task = { game_init_task, &game_init_params };
    task_scheduler->AddTask(task, ftl::TaskPriority::High);

    game_window_process_events(game_window);

    delete task_scheduler;

    if (game_init_params.game)
    {
        game_destroy(game_init_params.game);
    }

    game_window_destroy(game_window);

    return 0;
}
