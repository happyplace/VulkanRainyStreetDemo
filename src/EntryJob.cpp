#include "EntryJob.h"

#include "kiwi/KIWI_Scheduler.h"

#include "VulkanInitializeJobs.h"

void EntryJob(void* arg)
{
    KIWI_Job job { InitializeVulkanJob, nullptr };

    KIWI_Counter* counter = nullptr;
    KIWI_SchedulerAddJobs(&job, 1, KIWI_JobPriority_High, &counter);
    KIWI_SchedulerWaitForCounterAndFree(counter, 0);
}

