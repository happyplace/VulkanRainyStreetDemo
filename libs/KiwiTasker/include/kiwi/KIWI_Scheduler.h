#pragma once

#include "kiwi/KIWI_Std.h"
#include "kiwi/KIWI_Job.h"
#include "kiwi/KIWI_JobPriority.h"

#ifdef __cplusplus
extern "C" {
#endif

struct KIWI_Scheduler;
struct KIWI_Job;
struct KIWI_Counter;

typedef struct KIWI_SchedulerParams
{
	// this is the max amount of jobs that can be in queue
	int jobQueueSize;

	// this is the amount of worker threads to create.
	// setting a value higher than the physical cpu count or a value lower than one will be ignored, and will instead use the phyiscal cpu count.
	// KIWI_DefaultSchedulerParams will set this value to the physical cpu count
	int workerCount;

	// this is the max amount of jobs you can have running or waiting on counters to complete
	int fiberPoolSize;

	// the stack size to use for fibers
	int fiberStackSize;

	// max amount of task counters
	int countersCapacity;

	// max amount of fibers that can be waiting on a counter
	int waitingFiberCapacity;
} KIWI_SchedulerParams;

// this will populate params with default values. These defaults can be used to further tweak options
extern DECLSPEC void KIWI_DefaultSchedulerParams(KIWI_SchedulerParams* params);

// creates KIWI_Scheduler, initializes memory and starts worker threads. This function should be called before calling 
// any other functions on the Scheduler.
// KIWI_FreeScheduler needs to be called to shutdown worker threads and free memory
extern DECLSPEC struct KIWI_Scheduler* KIWI_CreateScheduler(const KIWI_SchedulerParams* params);

// shuts down worker threads and cleans up memory for scheduler
extern DECLSPEC void KIWI_FreeScheduler(struct KIWI_Scheduler* scheduler);

// Adds the job to the job queue at priority level. A copy of the job is kept so it's safe to delete the job definition. Any memory used by the argument
// needs to be valid until the job completes. 
// If counter is not NULL a counter will be created and used to track the job completion
extern DECLSPEC void KIWI_SchedulerAddJob(struct KIWI_Scheduler* scheduler, const KIWI_Job* job, const KIWI_JobPriority priority, struct KIWI_Counter** counter);

// Adds the jobs to the job queue at priority level. A copy of the job is kept so it's safe to delete the job definitions but any memory used by the argument 
// needs to be valid until the jobs completes.
// If counter is not NULL a counter will be created and used to track the jobs completion
extern DECLSPEC void KIWI_SchedulerAddJobs(struct KIWI_Scheduler* scheduler, const KIWI_Job* job, const int jobCount, const KIWI_JobPriority priority, struct KIWI_Counter** counter);

// frees a counter, this is only necessarily if you don't use WaitForCounterAndFree
// NOTE: do not free counters while there active jobs attached to it.
extern DECLSPEC void KIWI_SchedulerFreeCounter(struct KIWI_Scheduler* scheduler, struct KIWI_Counter* outCounter);

// halts the job until counter reaches targetValue and then returns.
// this function will also free the counter once it's completed
// NOTE: this function should not be called from outside of fibers
extern DECLSPEC void KIWI_SchedulerWaitForCounterAndFree(struct KIWI_Scheduler* scheduler, struct KIWI_Counter* counter, int targetValue);

// halts the job until counter reaches targetValue and then returns.
// this function WILL NOT free the counter, the programmer must free the counter
// NOTE: this function should not be called from outside of fibers
extern DECLSPEC void KIWI_SchedulerWaitForCounter(struct KIWI_Scheduler* scheduler, struct KIWI_Counter* counter, int targetValue);

#ifdef __cplusplus
}
#endif
