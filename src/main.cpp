#include "ftl/task_scheduler.h"
#include "ftl/wait_group.h"

#include "Game.h"
#include "GameWindow.h"

void fam_task(ftl::TaskScheduler* task_scheduler, void* arg)
{
    return;
}

int main(int argc, char** argv)
{
    GameWindow* game_window = game_window_init();
    if (game_window == nullptr)
    {
        return 1;
    }

    game_window_process_events(game_window);

    //return game_run(argc, argv);
    // ftl::TaskScheduler* task_scheduler = new ftl::TaskScheduler();
    // task_scheduler->Init();

    // ftl::Task task = { fam_task, nullptr };
    // task_scheduler->AddTask(task, ftl::TaskPriority::High);

    game_window_destroy(game_window);

    return 0;
}
