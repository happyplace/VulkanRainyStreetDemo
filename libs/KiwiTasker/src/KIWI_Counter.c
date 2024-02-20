#include "kiwi/KIWI_Counter.h"

#include <malloc.h>

#include "kiwi/KIWI_Std.h"
#include "kiwi/KIWI_SpinLock.h"
#include "kiwi/KIWI_Atomics.h"

typedef struct KIWI_CounterData
{
	atomic_int value;
	struct KIWI_SpinLock* lock;
} KIWI_CounterData;

struct KIWI_Counter* KIWI_CreateCounter()
{
	KIWI_Counter* counter = malloc(sizeof(KIWI_Counter));
	if (counter == NULL)
	{
		KIWI_ASSERT(!"are we out of memory???");
		return NULL;
	}

	KIWI_PrepareCounter(counter);

	return counter;
}

void KIWI_PrepareCounter(KIWI_Counter* counter)
{
	KIWI_ASSERT(counter);

	counter->data = malloc(sizeof(KIWI_CounterData));
	if (counter->data == NULL)
	{
		KIWI_ASSERT(!"are we out of memory???");
		return;
	}

	KIWI_CounterData* data = counter->data;
	data->lock = KIWI_CreateSpinLock();
	atomic_init(&data->value, 0);
}

void KIWI_FreeCounter(KIWI_Counter* counter)
{
	KIWI_ASSERT(counter);
	KIWI_ASSERT(counter->data);

	KIWI_FreePrepareCounter(counter);

	free(counter);
}

void KIWI_FreePrepareCounter(KIWI_Counter* counter)
{
	KIWI_ASSERT(counter);

	KIWI_CounterData* data = counter->data;
	KIWI_FreeSpinLock(data->lock);

	free(counter->data);
}

void KIWI_IncrementCounter(KIWI_Counter* counter)
{
	KIWI_ASSERT(counter);
	KIWI_ASSERT(counter->data);
	
	KIWI_CounterData* data = counter->data;

	KIWI_LockSpinLock(data->lock);
	atomic_fetch_add(&data->value, 1);
	KIWI_UnlockSpinLock(data->lock);
}

int KIWI_DecrementCounter(KIWI_Counter* counter)
{
	KIWI_ASSERT(counter);
	KIWI_ASSERT(counter->data);

	KIWI_CounterData* data = counter->data;

	KIWI_LockSpinLock(data->lock);
	int value = atomic_fetch_sub(&data->value, 1);
	value--;
	KIWI_UnlockSpinLock(data->lock);

	return value;
}

int KIWI_CounterLockAndGetValue(KIWI_Counter* counter)
{
	KIWI_ASSERT(counter);
	KIWI_ASSERT(counter->data);

	KIWI_CounterData* data = counter->data;

	KIWI_LockSpinLock(data->lock);
	return atomic_load(&data->value);
}

void KIWI_CounterUnlock(KIWI_Counter* counter)
{
	KIWI_ASSERT(counter);
	KIWI_ASSERT(counter->data);

	KIWI_CounterData* data = counter->data;

	KIWI_UnlockSpinLock(data->lock);
}

void KIWI_CounterResetToZero(KIWI_Counter* counter)
{
	KIWI_ASSERT(counter);
	KIWI_ASSERT(counter->data);

	KIWI_CounterData* data = counter->data;
	atomic_store(&data->value, 0);
}
