#include "kiwi/KIWI_Scheduler.h"

#include <malloc.h>
#include <string.h>

#include "kiwi/KIWI_ThreadImpl.h"
#include "kiwi/KIWI_FiberWorkerStorage.h"
#include "kiwi/KIWI_Atomics.h"
#include "kiwi/KIWI_SpinLock.h"
#include "kiwi/KIWI_Queue.h"
#include "kiwi/KIWI_FiberPool.h"
#include "kiwi/KIWI_Array.h"
#include "kiwi/KIWI_Counter.h"
#include "kiwi/KIWI_CounterPool.h"

#ifdef KIWI_ENABLE_VALGRIND_SUPPORT
	#if __has_include(<valgrind/valgrind.h>)
		#define KIWI_HAS_VALGRIND
		#include <valgrind/valgrind.h>
	#endif // __has_include(<valgrind/valgrind.h>)
#endif 

typedef struct KIWI_Scheduler
{
	struct KIWI_ThreadImpl* threadImpl;
	atomic_bool quitWorkerThreads;

	struct KIWI_SpinLock* queueLock;
	struct KIWI_Queue* queueLow;
	struct KIWI_Queue* queueNormal;
	struct KIWI_Queue* queueHigh;
	struct KIWI_Queue* queueReadyFibers;

	struct KIWI_SpinLock* waitingFibersLock;
	struct KIWI_Array* waitingFibers;

	struct KIWI_FiberPool* fiberPool;
	struct KIWI_CounterPool* counterPool;
} KIWI_Scheduler;

typedef struct KIWI_CounterContainer
{
	struct KIWI_Counter* counter;
} KIWI_CounterContainer;

typedef struct KIWI_PendingJob
{
	KIWI_Job job;
	struct KIWI_CounterContainer counter;
} KIWI_PendingJob;

typedef struct KIWI_WaitingFiber
{
	struct KIWI_Fiber* fiber;
	struct KIWI_Counter* counter;
	int targetValue;
} KIWI_WaitingFiber;

static KIWI_Scheduler* s_scheduler = NULL;

void KIWI_DefaultSchedulerParams(KIWI_SchedulerParams* params)
{
	KIWI_ASSERT(params);

	params->jobQueueSize = 1024;
	params->workerCount = KIWI_ThreadImplGetCpuCount();
	params->fiberPoolSize = 512;
	params->fiberStackSize = 512 * 1024;
	params->countersCapacity = 25;
	params->waitingFiberCapacity = 128;
}

bool KIWI_SchedulerGetNextFiber(KIWI_Fiber** outFiber, bool* outResumeFiber)
{
	(*outResumeFiber) = false;

	KIWI_LockSpinLock(s_scheduler->queueLock);

	if (!KIWI_QueueIsEmpty(s_scheduler->queueHigh))
	{
		KIWI_PendingJob pendingJob;
		bool result = KIWI_QueuePop(s_scheduler->queueHigh, &pendingJob);
		KIWI_ASSERT(result);
		KIWI_ALLOW_UNUSED(result);

		KIWI_Fiber* fiber = KIWI_FiberPoolGet(s_scheduler->fiberPool);
		(*outFiber) = fiber;
		fiber->job = pendingJob.job;
		fiber->counter = pendingJob.counter.counter;

		KIWI_UnlockSpinLock(s_scheduler->queueLock);
		return true;		
	}
	else if (!KIWI_QueueIsEmpty(s_scheduler->queueReadyFibers))
	{
		KIWI_Fiber* fiber = NULL;
		bool result = KIWI_QueuePop(s_scheduler->queueReadyFibers, &fiber);
		KIWI_ASSERT(result);
		KIWI_ALLOW_UNUSED(result);

		(*outResumeFiber) = true;
		(*outFiber) = fiber;

		KIWI_UnlockSpinLock(s_scheduler->queueLock);
		return true;
	}
	else if (!KIWI_QueueIsEmpty(s_scheduler->queueNormal))
	{
		KIWI_PendingJob pendingJob;
		bool result = KIWI_QueuePop(s_scheduler->queueNormal, &pendingJob);
		KIWI_ASSERT(result);
		KIWI_ALLOW_UNUSED(result);

		KIWI_Fiber* fiber = KIWI_FiberPoolGet(s_scheduler->fiberPool);
		(*outFiber) = fiber;
		fiber->job = pendingJob.job;
		fiber->counter = pendingJob.counter.counter;

		KIWI_UnlockSpinLock(s_scheduler->queueLock);
		return true;
	}
	else if (!KIWI_QueueIsEmpty(s_scheduler->queueLow))
	{
		KIWI_PendingJob pendingJob;
		bool result = KIWI_QueuePop(s_scheduler->queueLow, &pendingJob);
		KIWI_ASSERT(result);
		KIWI_ALLOW_UNUSED(result);

		KIWI_Fiber* fiber = KIWI_FiberPoolGet(s_scheduler->fiberPool);
		(*outFiber) = fiber;
		fiber->job = pendingJob.job;
		fiber->counter = pendingJob.counter.counter;

		KIWI_UnlockSpinLock(s_scheduler->queueLock);
		return true;
	}
	else
	{
		KIWI_UnlockSpinLock(s_scheduler->queueLock);
		return false;
	}
}

void KIWI_FiberEntry(fcontext_transfer_t t)
{	
	KIWI_FiberWorkerStorage* workerStorage = KIWI_GetFiberWorkerStorage(KIWI_ThreadImplGetWorkerThreadIndex());
	KIWI_Fiber* fiber = workerStorage->fiber;
	
	fiber->context = t;

	fiber->job.entry(workerStorage->scheduler, fiber->job.arg);
	
	// updating worker storage and fiber because we could be on another thread now
	workerStorage = KIWI_GetFiberWorkerStorage(KIWI_ThreadImplGetWorkerThreadIndex());
	fiber = workerStorage->fiber;

	workerStorage->completedFiber = true;
	jump_fcontext(fiber->context.ctx, NULL);
}

void KIWI_AddFiberToReadyQueue(KIWI_Fiber* fiber)
{
	KIWI_LockSpinLock(s_scheduler->queueLock);
	KIWI_QueuePush(s_scheduler->queueReadyFibers, &fiber);
	KIWI_UnlockSpinLock(s_scheduler->queueLock);

	KIWI_ThreadImplNotifyOneWorkerThread(s_scheduler->threadImpl);
}

void KIWI_CheckWaitingFibers(struct KIWI_Counter* counter, int value)
{
	KIWI_LockSpinLock(s_scheduler->waitingFibersLock);
	int size = KIWI_ArraySize(s_scheduler->waitingFibers);
	for (int i = 0; i < size; ++i)
	{
		KIWI_WaitingFiber* waitingFiber = KIWI_ArrayGet(s_scheduler->waitingFibers, i);
		if (waitingFiber->counter == counter)
		{
			if (waitingFiber->targetValue == value)
			{
				KIWI_ArrayRemoveItem(s_scheduler->waitingFibers, i);
				i--;
				size--;

				KIWI_AddFiberToReadyQueue(waitingFiber->fiber);
			}
		}
	}
	KIWI_UnlockSpinLock(s_scheduler->waitingFibersLock);
}

WORKER_THREAD_DEFINITION(arg)
{
	KIWI_ALLOW_UNUSED(arg);

	KIWI_ThreadImplBlockSignalsOnWorkerThread();

	KIWI_FiberWorkerStorage* workerStorage = KIWI_GetFiberWorkerStorage(KIWI_ThreadImplGetWorkerThreadIndex());

	while (!atomic_load(KIWI_GetFiberWorkerStorage(KIWI_ThreadImplGetWorkerThreadIndex())->quitWorkerThreads))
	{
		KIWI_Fiber* fiber = NULL;
		bool resumeFiber = false;
		if (KIWI_SchedulerGetNextFiber(&fiber, &resumeFiber))
		{
			workerStorage->fiber = fiber;
			
			if (resumeFiber)
			{
				// we're going back in
				fiber->context = jump_fcontext(fiber->context.ctx, NULL);
			}
			else
			{
				// this is a new fiber, we need to create new context 
				fcontext_t fiberEntry = make_fcontext(fiber->stack.sptr, fiber->stack.ssize, KIWI_FiberEntry);

#if defined(KIWI_HAS_VALGRIND)
                // Before context switch, register our stack with Valgrind.
                fiber->stackId = VALGRIND_STACK_REGISTER((char*)fiber->stack.sptr - fiber->stack.ssize, fiber->stack.sptr);
#endif // defined(KIWI_HAS_VALGRIND)

				// make the jump, see you on the other side
				fiber->context = jump_fcontext(fiberEntry, NULL);
			}

			// we could be returning on a different thread, refresh worker storage and fiber
			workerStorage = KIWI_GetFiberWorkerStorage(KIWI_ThreadImplGetWorkerThreadIndex());
			fiber = workerStorage->fiber;

			// if we're coming back here we need to check if the fiber actually completed
			if (workerStorage->completedFiber)
			{
				if (fiber->counter)
				{
					int value = KIWI_DecrementCounter(fiber->counter);
					KIWI_CheckWaitingFibers(fiber->counter, value);
				}

#if defined(KIWI_HAS_VALGRIND)
                // Before we give back the fiber we deregister our stack with Valgrind.
                VALGRIND_STACK_DEREGISTER(fiber->stackId);
#endif // defined(KIWI_HAS_VALGRIND)
				KIWI_FiberPoolReturn(workerStorage->scheduler->fiberPool, fiber);
			}
			else
			{
				// we can release the counter lock now because the fiber can be successfully resumed
				struct KIWI_Counter* counter = fiber->context.data;
				KIWI_CounterUnlock(counter);
			}
		}
		else
		{	
			workerStorage = KIWI_GetFiberWorkerStorage(KIWI_ThreadImplGetWorkerThreadIndex());
			KIWI_ThreadImplSleepUntilJobAdded(workerStorage->threadImpl, workerStorage->quitWorkerThreads);
		}
	}

	WORKER_THREAD_RETURN_STATEMENT;
}

int KIWI_InitScheduler(const KIWI_SchedulerParams* params)
{
	s_scheduler = malloc(sizeof(KIWI_Scheduler));
	if (s_scheduler == NULL)
	{
		KIWI_ASSERT(!"are we out of memory???");
		return 0;
	}

	// TODO: we're calling a lot of functions that alloc memory and we aren't checking for failure

	s_scheduler->queueLock = KIWI_CreateSpinLock();

	s_scheduler->queueLow = KIWI_CreateQueue(sizeof(KIWI_PendingJob), params->jobQueueSize);
	s_scheduler->queueNormal = KIWI_CreateQueue(sizeof(KIWI_PendingJob), params->jobQueueSize);
	s_scheduler->queueHigh = KIWI_CreateQueue(sizeof(KIWI_PendingJob), params->jobQueueSize);

	s_scheduler->queueReadyFibers = KIWI_CreateQueue(sizeof(KIWI_Fiber*), params->waitingFiberCapacity);
	
	s_scheduler->waitingFibersLock = KIWI_CreateSpinLock();
	s_scheduler->waitingFibers = KIWI_CreateArray(sizeof(KIWI_WaitingFiber), params->waitingFiberCapacity);

	int cpuCount = KIWI_ThreadImplGetCpuCount();
	if (params->workerCount < cpuCount && params->workerCount >= 1)
	{
		cpuCount = params->workerCount;
	}
	
	s_scheduler->fiberPool = KIWI_CreateFiberPool(params->fiberPoolSize, params->fiberStackSize);

	s_scheduler->counterPool = KIWI_CreateCounterPool(params->countersCapacity);

	KIWI_CreateFiberWorkerStorage(cpuCount);
	for (int i = 0; i < cpuCount; ++i)
	{
		KIWI_FiberWorkerStorage* workerStorage = KIWI_GetFiberWorkerStorage(i);
		workerStorage->quitWorkerThreads = &s_scheduler->quitWorkerThreads;
		workerStorage->scheduler = s_scheduler;
	}

	atomic_store(&s_scheduler->quitWorkerThreads, false);
	s_scheduler->threadImpl = KIWI_ThreadImplCreateAndStartWorkerThreads(cpuCount, SchedulerWorkerThread);

	return 1;
}

void KIWI_FreeScheduler()
{
	KIWI_ASSERT(s_scheduler);

	KIWI_ThreadImplSignalWorkerThreadsToQuit(s_scheduler->threadImpl, &s_scheduler->quitWorkerThreads);
	KIWI_ThreadImplNotifyAllWorkerThreads(s_scheduler->threadImpl);
	KIWI_ThreadImplShutdownWorkerThreads(s_scheduler->threadImpl);

	KIWI_DestroyFiberWorkerStorage();

	KIWI_FreeSpinLock(s_scheduler->queueLock);
	KIWI_FreeQueue(s_scheduler->queueLow);
	KIWI_FreeQueue(s_scheduler->queueNormal);
	KIWI_FreeQueue(s_scheduler->queueHigh);
	KIWI_FreeQueue(s_scheduler->queueReadyFibers);

	KIWI_FreeSpinLock(s_scheduler->waitingFibersLock);
	KIWI_FreeArray(s_scheduler->waitingFibers);

	KIWI_FreeFiberPool(s_scheduler->fiberPool);

	KIWI_FreeCounterPool(s_scheduler->counterPool);

	free(s_scheduler);
	s_scheduler = NULL;
}

void KIWI_SchedulerAddJob(const KIWI_Job* job, const KIWI_JobPriority priority, struct KIWI_Counter** counter)
{
	KIWI_ASSERT(s_scheduler);
	KIWI_ASSERT(job);

	KIWI_SchedulerAddJobs(job, 1, priority, counter);
}

struct KIWI_Counter* KIWI_SchedulerCreateCounter()
{
	KIWI_ASSERT(s_scheduler);

	struct KIWI_Counter* counter = KIWI_CounterPoolGet(s_scheduler->counterPool);

	return counter;
}

void KIWI_SchedulerFreeCounter(struct KIWI_Counter* counter)
{
	KIWI_ASSERT(s_scheduler);
	KIWI_ASSERT(counter);

	KIWI_CounterPoolReturn(s_scheduler->counterPool, counter);
}

void KIWI_SchedulerAddJobs(const KIWI_Job* job, const int jobCount, const KIWI_JobPriority priority, struct KIWI_Counter** outCounter)
{	
	KIWI_ASSERT(s_scheduler);
	KIWI_ASSERT(job);
	KIWI_ASSERT(jobCount >= 1);

	struct KIWI_Queue* queue = NULL;
	switch (priority)
	{
	case KIWI_JobPriority_High:
		queue = s_scheduler->queueHigh;
		break;
	case KIWI_JobPriority_Normal:
		queue = s_scheduler->queueNormal;
		break;
	case KIWI_JobPriority_Low:
		queue = s_scheduler->queueLow;
		break;
	default:
		KIWI_ASSERT(!"Unknown job priority, normal priority will be used");
		queue = s_scheduler->queueNormal;
		break;
	}

	struct KIWI_Counter* counter = NULL;
	if (outCounter != NULL)
	{
		if ((*outCounter) != NULL)
		{
			// they're reusing a counter so we don't need to create one
			counter = (*outCounter);
		}
		else
		{
			counter = KIWI_SchedulerCreateCounter();
			(*outCounter) = counter;
		}
	}

	KIWI_LockSpinLock(s_scheduler->queueLock);
	for (int i = 0; i < jobCount; ++i)
	{
		KIWI_PendingJob pendingJob;
		memcpy(&pendingJob.job, ((char*)job + (sizeof(KIWI_Job) * i)), sizeof(KIWI_Job));
		pendingJob.counter.counter = counter;
		KIWI_QueuePush(queue, &pendingJob);
		
		if (counter)
		{
			KIWI_IncrementCounter(counter);
		}
	}
	KIWI_UnlockSpinLock(s_scheduler->queueLock);

	if (jobCount == 1)
	{
		KIWI_ThreadImplNotifyOneWorkerThread(s_scheduler->threadImpl);
	}
	else
	{
		KIWI_ThreadImplNotifyAllWorkerThreads(s_scheduler->threadImpl);
	}
}

void KIWI_SchedulerWaitForCounterAndFree(struct KIWI_Counter* counter, int targetValue)
{
	KIWI_SchedulerWaitForCounter(counter, targetValue);

	// when we return the jobs should be complete so we can resume
	KIWI_SchedulerFreeCounter(counter);
}

void KIWI_SchedulerWaitForCounter(struct KIWI_Counter* counter, int targetValue)
{
	KIWI_ASSERT(s_scheduler);
	KIWI_ASSERT(counter);
	KIWI_ASSERT(targetValue >= 0);

	KIWI_FiberWorkerStorage* workerStorage = KIWI_GetFiberWorkerStorage(KIWI_ThreadImplGetWorkerThreadIndex());

	KIWI_WaitingFiber waitingFiber;
	waitingFiber.fiber = workerStorage->fiber;
	waitingFiber.targetValue = targetValue;
	waitingFiber.counter = counter;

	int counterValue = KIWI_CounterLockAndGetValue(counter);
	if (counterValue == targetValue)
	{
		KIWI_CounterUnlock(counter);
		return;
	}
	
	KIWI_LockSpinLock(s_scheduler->waitingFibersLock);
	KIWI_ArrayAddItem(s_scheduler->waitingFibers, &waitingFiber);
	KIWI_UnlockSpinLock(s_scheduler->waitingFibersLock);

	workerStorage->completedFiber = false;

	// because the fiber's context is pointing at the context to return here we can't unlock the counter
	// so we send the counter so we can unlock it when we get back to the worker thread main function.
	fcontext_transfer_t returnTransfer = jump_fcontext(workerStorage->fiber->context.ctx, counter);

	workerStorage = KIWI_GetFiberWorkerStorage(KIWI_ThreadImplGetWorkerThreadIndex());
	workerStorage->fiber->context = returnTransfer;
}
