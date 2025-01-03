#include "ftl/task_scheduler.h"

#include "Game.h"
#include "GameWindow.h"

int main(int argc, char** argv)
{
    GameWindow* game_window = game_window_init();
    if (game_window == nullptr)
    {
        return 1;
    }

    ftl::TaskSchedulerInitOptions options;
    options.Behavior = ftl::EmptyQueueBehavior::Sleep;
    options.ThreadPoolSize = 4;

    ftl::TaskScheduler* task_scheduler = new ftl::TaskScheduler();
    task_scheduler->Init(options);

    GameInitParams game_init_params;
    game_init_params.argc = argc;
    game_init_params.argv = argv;
    game_init_params.game = nullptr;
    game_init_params.game_window = game_window;
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
