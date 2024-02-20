#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct KIWI_Scheduler;

typedef void (*KIWI_JobEntry)(struct KIWI_Scheduler*, void*);

typedef struct KIWI_Job
{
	KIWI_JobEntry entry;
	void* arg;
} KIWI_Job;

#ifdef __cplusplus
}
#endif
