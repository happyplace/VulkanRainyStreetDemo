#pragma once

#include <stdbool.h>

#include "kiwi/KIWI_Std.h"
#include "kiwi/KIWI_Atomics.h"

#ifdef __cplusplus
extern "C" {
#endif

struct KIWI_ThreadImpl;
struct KIWI_Scheduler;
struct KIWI_Fiber;

typedef struct KIWI_FiberWorkerStorage
{
    struct KIWI_ThreadImpl* threadImpl;    
    struct KIWI_Scheduler* scheduler;
    atomic_bool* quitWorkerThreads;
    struct KIWI_Fiber* fiber;
    bool completedFiber;
} KIWI_FiberWorkerStorage;

void KIWI_CreateFiberWorkerStorage(int cpuCount);
void KIWI_DestroyFiberWorkerStorage();
KIWI_FiberWorkerStorage* KIWI_GetFiberWorkerStorage(int cpuIndex);

#ifdef __cplusplus
}
#endif
