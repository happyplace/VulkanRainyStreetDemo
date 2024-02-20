#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct KIWI_CounterPool;
struct KIWI_Counter;

// creates a pool of fibers of size poolSize. The Object returned by this function needs to be freed with KIWI_FreeFiberPool
struct KIWI_CounterPool* KIWI_CreateCounterPool(int poolSize);

// frees all memory associated with a fiber pool
void KIWI_FreeCounterPool(struct KIWI_CounterPool* counterPool);

// returns the next free fiber in the pool
struct KIWI_Counter* KIWI_CounterPoolGet(struct KIWI_CounterPool* counterPool);

// returns fiber to pool
void KIWI_CounterPoolReturn(struct KIWI_CounterPool* counterPool, struct KIWI_Counter* counter);

#ifdef __cplusplus
}
#endif
