#include "kiwi/KIWI_SpinLock.h"

#include <malloc.h>

#include "kiwi/KIWI_Std.h"
#include "kiwi/KIWI_Atomics.h"

typedef struct KIWI_SpinLock
{
	atomic_flag flag;
} KIWI_SpinLock;

// Creates a spin lock, the memory returned needs to be freed using KIWI_FreeSpinLock when done with the lock
struct KIWI_SpinLock* KIWI_CreateSpinLock()
{
	KIWI_SpinLock* lock = malloc(sizeof(KIWI_SpinLock));
	if (lock == NULL)
	{
		KIWI_ASSERT(!"are we out of memory???");
		return NULL;
	}

	atomic_flag flag = ATOMIC_FLAG_INIT;
	lock->flag = flag;
	return lock;
}

// frees memory of a spin lock
void KIWI_FreeSpinLock(struct KIWI_SpinLock* lock)
{
	KIWI_ASSERT(lock);

	free(lock);
}

void KIWI_LockSpinLock(struct KIWI_SpinLock* lock)
{
	while (atomic_flag_test_and_set(&lock->flag))
		;
}

void KIWI_UnlockSpinLock(struct KIWI_SpinLock* lock)
{
	atomic_flag_clear(&lock->flag);
}
