#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*KIWI_JobEntry)(void*);

typedef struct KIWI_Job
{
	KIWI_JobEntry entry;
	void* arg;
} KIWI_Job;

#ifdef __cplusplus
}
#endif
