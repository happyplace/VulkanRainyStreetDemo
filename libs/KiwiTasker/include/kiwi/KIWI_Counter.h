#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct KIWI_Counter
{
	void* data;
	struct KIWI_Counter* next;
} KIWI_Counter;

// Creates a counter, the memory returned needs to be freed using KIWI_FreeCounter when done with it
KIWI_Counter* KIWI_CreateCounter();

// works the same as CreateCounter but assumes the memory for KIWI_Counter has already been allocated.
// the primary purpose of this function is for use with CounterPool
void KIWI_PrepareCounter(KIWI_Counter* counter);

// frees memory of a spin lock
void KIWI_FreeCounter(KIWI_Counter* counter);

// this is the free function that should be used if the counter had KIWI_PrepareCounter called on it
void KIWI_FreePrepareCounter(KIWI_Counter* counter);

// This function is thread-safe
void KIWI_IncrementCounter(KIWI_Counter* counter);

// returns the value that this call decremented the counter to, this won't necessarily reflect the current
// value of the counter.
// This function is thread-safe
int KIWI_DecrementCounter(KIWI_Counter* counter);

// locks the counter and gets the current value, this should only be used when a job is being added to the waitList
int KIWI_CounterLockAndGetValue(KIWI_Counter* counter);

// this unlocks the counter. This should be called after the job has been added to the wait list
void KIWI_CounterUnlock(KIWI_Counter* counter);

// this function is used to set the counter back to zero when counters are used in counter pools
// this function IS NOT thread-safe
void KIWI_CounterResetToZero(KIWI_Counter* counter);

#ifdef __cplusplus
}
#endif
