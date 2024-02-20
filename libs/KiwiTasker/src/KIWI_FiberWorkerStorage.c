#include "kiwi/KIWI_FiberWorkerStorage.h"

#include <malloc.h>

#include "kiwi/KIWI_Std.h"

static KIWI_FiberWorkerStorage* fiber_worker_storage = NULL;

void KIWI_CreateFiberWorkerStorage(int cpuCount)
{
    KIWI_ASSERT(fiber_worker_storage == NULL && "are you calling this function more than once or is there more than one KIWI_Scheduler in existance?");

    fiber_worker_storage = malloc(sizeof(KIWI_FiberWorkerStorage) * cpuCount);
}

void KIWI_DestroyFiberWorkerStorage()
{
    KIWI_ASSERT(fiber_worker_storage != NULL);
    if (fiber_worker_storage != NULL)
    {
        free(fiber_worker_storage);
    }
}

KIWI_FiberWorkerStorage* KIWI_GetFiberWorkerStorage(int cpuIndex)
{
    if (cpuIndex < 0)
    {
        return NULL;
    }

    return (KIWI_FiberWorkerStorage*)((char*)fiber_worker_storage + ((sizeof(KIWI_FiberWorkerStorage)) * cpuIndex));
}
    