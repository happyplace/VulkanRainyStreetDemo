#include "kiwi/KIWI_FiberPool.h"

#include <malloc.h>

#include "kiwi/KIWI_Std.h"
#include "kiwi/KIWI_SpinLock.h"

typedef struct KIWI_FiberPool
{
	KIWI_Fiber* fibers;
	KIWI_Fiber* firstFree;
	struct KIWI_SpinLock* lock;
	int poolSize;
} KIWI_FiberPool;

KIWI_Fiber* KIWI_GetFiberElement(KIWI_FiberPool* pool, int index)
{
	KIWI_ASSERT(pool);
	KIWI_ASSERT(index >= 0 && "invalid index");

	if (index < 0)
	{
		return NULL;
	}

	return (KIWI_Fiber*)((char*)pool->fibers + (sizeof(KIWI_Fiber) * index));
}

struct KIWI_FiberPool* KIWI_CreateFiberPool(int poolSize, int stackSize)
{
	KIWI_ASSERT(poolSize >= 1 && "pool size has to be greater than zero");

	KIWI_FiberPool* fiberPool = malloc(sizeof(KIWI_FiberPool));
	if (fiberPool == NULL)
	{
		KIWI_ASSERT(!"are we out of memory???");
		return NULL;
	}

	fiberPool->poolSize = poolSize;
	fiberPool->lock = KIWI_CreateSpinLock();

	fiberPool->fibers = malloc(sizeof(KIWI_Fiber) * poolSize);
	if (fiberPool->fibers == NULL)
	{
		KIWI_ASSERT(!"are we out of memory???");
		return NULL;
	}	

	for (int i = 0; i < poolSize - 1; ++i)
	{		
		KIWI_Fiber* element = KIWI_GetFiberElement(fiberPool, i);
		element->stack = create_fcontext_stack(stackSize);
		element->next = KIWI_GetFiberElement(fiberPool, i + 1);		
	}

	KIWI_Fiber* last = KIWI_GetFiberElement(fiberPool, poolSize - 1);
	last->stack = create_fcontext_stack(stackSize);
	last->next = NULL;

	fiberPool->firstFree = KIWI_GetFiberElement(fiberPool, 0);

	return fiberPool;
}

void KIWI_FreeFiberPool(struct KIWI_FiberPool* fiberPool)
{
	KIWI_ASSERT(fiberPool);

	for (int i = 0; i < fiberPool->poolSize; ++i)
	{		
		KIWI_Fiber* element = KIWI_GetFiberElement(fiberPool, i);
		destroy_fcontext_stack(&element->stack);
	}

	KIWI_FreeSpinLock(fiberPool->lock);

	free(fiberPool->fibers);
	free(fiberPool);
}

KIWI_Fiber* KIWI_FiberPoolGet(struct KIWI_FiberPool* fiberPool)
{
	KIWI_ASSERT(fiberPool);

	KIWI_Fiber* fiber = NULL;

	KIWI_LockSpinLock(fiberPool->lock);
	if (fiberPool->firstFree != NULL)
	{
		fiber = fiberPool->firstFree;
		fiberPool->firstFree = fiberPool->firstFree->next;
	}
	KIWI_UnlockSpinLock(fiberPool->lock);

	KIWI_ASSERT(fiber && "no more free fibers in pool increase pool size");

	return fiber;
}

void KIWI_FiberPoolReturn(struct KIWI_FiberPool* fiberPool, KIWI_Fiber* fiber)
{
	KIWI_ASSERT(fiberPool);
	KIWI_ASSERT(fiber);

	KIWI_LockSpinLock(fiberPool->lock);
	fiber->next = fiberPool->firstFree;
	fiberPool->firstFree = fiber;
	KIWI_UnlockSpinLock(fiberPool->lock);	
}
