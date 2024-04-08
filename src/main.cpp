#include "ftl/task_scheduler.h"

#include "Window.h"
#include "Singleton.h"
#include "GameCommandParameters.h"
#include "EntryJob.h"

int main(int argc, char** argv)
{
    ftl::TaskScheduler taskScheduler;
    taskScheduler.Init();

    Singleton<GameCommandParameters>::Create();
    Singleton<GameCommandParameters>::Get()->SetValues(argc, argv);

    Singleton<Window>::Create();
    if (!Singleton<Window>::Get()->Init())
    {
        return 1;
    }

    ftl::Task task = { EntryJob, nullptr };
    taskScheduler.AddTask(task, ftl::TaskPriority::High);

    Singleton<Window>::Get()->WaitAndProcessEvents();

    Singleton<Window>::Destroy();
    Singleton<GameCommandParameters>::Destroy();

    return 0;
}
