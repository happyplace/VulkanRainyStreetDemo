#include <iostream>

#include "kiwi/KIWI_Scheduler.h"

#include "Window.h"
#include "EntryJob.h"
#include "Singleton.h"

int main(int argc, char** argv)
{
    KIWI_SchedulerParams params;
    KIWI_DefaultSchedulerParams(&params);

    KIWI_InitScheduler(&params);

    Singleton<Window>::Create();
    if (!Singleton<Window>::Get()->Init(argc, argv))
    {
        return 1;
    }

    KIWI_Job firstJob { EntryJob, nullptr };
    KIWI_SchedulerAddJob(&firstJob, KIWI_JobPriority_High, nullptr);

    Singleton<Window>::Get()->WaitAndProcessEvents();

    KIWI_FreeScheduler();

    Singleton<Window>::Destroy();

    return 0;
}
