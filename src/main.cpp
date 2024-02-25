#include <iostream>

#include "kiwi/KIWI_Scheduler.h"

#include "Window.h"
#include "EntryJob.h"

int main(int argc, char** argv)
{
    KIWI_SchedulerParams params;
    KIWI_DefaultSchedulerParams(&params);

    KIWI_InitScheduler(&params);

    Window window;
    if (!window.Init(argc, argv))
    {
        return 1;
    }

    KIWI_Job firstJob { EntryJob, nullptr };
    KIWI_SchedulerAddJob(&firstJob, KIWI_JobPriority_High, nullptr);

    window.WaitAndProcessEvents();

    KIWI_FreeScheduler();

    return 0;
}
