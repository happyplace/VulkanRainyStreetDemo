#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct KIWI_Queue;

// creates a queue with a max capacity of maxCapacity and a element size of elementSize. returns NULL on error.
// The pointer returned by this function needs a call to KIWI_FreeQueue to free the memory
struct KIWI_Queue* KIWI_CreateQueue(int elementSize, int maxCapacity);

// This frees all the memory associated with a queue created by KIWI_CreateQueue
void KIWI_FreeQueue(struct KIWI_Queue* queue);

// returns true if the queue is empty
bool KIWI_QueueIsEmpty(struct KIWI_Queue* queue);

// returns true if the queue is full
bool KIWI_QueueIsFull(struct KIWI_Queue* queue);

// pushes value into the end of queue
void KIWI_QueuePush(struct KIWI_Queue* queue, void* value);

// pops the first value in the queue and copies it into outValue.
// NOTE: outValue is assumed to be at least elementSize
// returns true if a value is found or false if the queue is empty
bool KIWI_QueuePop(struct KIWI_Queue* queue, void* outValue);

#ifdef __cplusplus
}
#endif
