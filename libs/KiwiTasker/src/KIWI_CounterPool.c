#include "kiwi/KIWI_CounterPool.h"

#include <malloc.h>

#include "kiwi/KIWI_Std.h"
#include "kiwi/KIWI_SpinLock.h"
#include "kiwi/KIWI_Counter.h"

typedef struct KIWI_CounterPool
{
	struct KIWI_Counter* counters;
	struct KIWI_Counter* firstFree;
	struct KIWI_SpinLock* lock;
	int poolSize;
} KIWI_CounterPool;

struct KIWI_Counter* KIWI_GetCounterElement(KIWI_CounterPool* pool, int index)
{
	KIWI_ASSERT(pool);
	KIWI_ASSERT(index >= 0 && "invalid index");

	if (index < 0)
	{
		return NULL;
	}

	return (KIWI_Counter*)((char*)pool->counters + (sizeof(KIWI_Counter) * index));
}

struct KIWI_CounterPool* KIWI_CreateCounterPool(int poolSize)
{
	KIWI_ASSERT(poolSize >= 1 && "pool size has to be greater than zero");

	KIWI_CounterPool* counterPool = malloc(sizeof(KIWI_CounterPool));
	if (counterPool == NULL)
	{
		KIWI_ASSERT(!"are we out of memory???");
		return NULL;
	}

	counterPool->poolSize = poolSize;
	counterPool->lock = KIWI_CreateSpinLock();

	counterPool->counters = malloc(sizeof(KIWI_Counter) * poolSize);
	if (counterPool->counters == NULL)
	{
		KIWI_ASSERT(!"are we out of memory???");
		return NULL;
	}

	for (int i = 0; i < poolSize - 1; ++i)
	{
		KIWI_Counter* counter = KIWI_GetCounterElement(counterPool, i);
		KIWI_PrepareCounter(counter);
		counter->next = KIWI_GetCounterElement(counterPool, i + 1);
	}

	KIWI_Counter* last = KIWI_GetCounterElement(counterPool, poolSize - 1);
	KIWI_PrepareCounter(last);
	last->next = NULL;

	counterPool->firstFree = KIWI_GetCounterElement(counterPool, 0);

	return counterPool;
}

void KIWI_FreeCounterPool(struct KIWI_CounterPool* counterPool)
{
	KIWI_ASSERT(counterPool);

	for (int i = 0; i < counterPool->poolSize; ++i)
	{
		KIWI_Counter* counter = KIWI_GetCounterElement(counterPool, i);
		KIWI_FreePrepareCounter(counter);
	}

	KIWI_FreeSpinLock(counterPool->lock);

	free(counterPool->counters);
	free(counterPool);
}

struct KIWI_Counter* KIWI_CounterPoolGet(struct KIWI_CounterPool* counterPool)
{
	KIWI_ASSERT(counterPool);

	KIWI_Counter* counter = NULL;

	KIWI_LockSpinLock(counterPool->lock);
	if (counterPool->firstFree != NULL)
	{
		counter = counterPool->firstFree;
		counterPool->firstFree = counterPool->firstFree->next;
	}
	KIWI_UnlockSpinLock(counterPool->lock);

	KIWI_ASSERT(counter && "no more free counters in pool increase pool size");

	KIWI_CounterResetToZero(counter);

	return counter;
}

void KIWI_CounterPoolReturn(struct KIWI_CounterPool* counterPool, struct KIWI_Counter* counter)
{
	KIWI_ASSERT(counterPool);
	KIWI_ASSERT(counter);

	KIWI_LockSpinLock(counterPool->lock);
	counter->next = counterPool->firstFree;
	counterPool->firstFree = counter;
	KIWI_UnlockSpinLock(counterPool->lock);
}
