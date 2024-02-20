#include "kiwi/KIWI_Queue.h"

#include <malloc.h>
#include <string.h>

#include "kiwi/KIWI_Std.h"

typedef struct KIWI_Queue
{
	void* data;
	int elementSize;
	int maxCapacity;
	int headIndex;
	int rearIndex;
} KIWI_Queue;

void* KIWI_GetQueueElement(KIWI_Queue* queue, int index)
{
	KIWI_ASSERT(queue);
	KIWI_ASSERT(index >= 0 && "invalid index");

	if (index < 0)
	{
		return NULL;
	}

	return (char*)queue->data + ((size_t)queue->elementSize * index);
}

struct KIWI_Queue* KIWI_CreateQueue(int elementSize, int maxCapacity)
{
	KIWI_ASSERT(elementSize > 0);
	KIWI_ASSERT(maxCapacity > 0);

	KIWI_Queue* queue = malloc(sizeof(KIWI_Queue));
	if (queue == NULL)
	{		
		KIWI_ASSERT(!"are we out of memory???");
		return NULL;
	}
	
	queue->elementSize = elementSize;
	queue->maxCapacity = maxCapacity;
	queue->headIndex = -1;
	queue->rearIndex = 0;

	queue->data = malloc((size_t)queue->elementSize * queue->maxCapacity);
	if (queue->data == NULL)
	{
		KIWI_ASSERT(!"are we out of memory???");
		free(queue);
		return NULL;
	}

	return queue;
}
  
void KIWI_FreeQueue(KIWI_Queue* queue)
{
	KIWI_ASSERT(queue);

	free(queue->data);
	free(queue);
}

bool KIWI_QueueIsEmpty(struct KIWI_Queue* queue)
{
	KIWI_ASSERT(queue);

	return queue->headIndex == -1;
}

bool KIWI_QueueIsFull(struct KIWI_Queue* queue)
{
	KIWI_ASSERT(queue);

	return queue->rearIndex == queue->headIndex;
}

void KIWI_QueuePush(struct KIWI_Queue* queue, void* value)
{
	KIWI_ASSERT(queue);
	KIWI_ASSERT(queue->rearIndex != queue->headIndex && "the queue is full");

	void* element = KIWI_GetQueueElement(queue, queue->rearIndex);
	memcpy(element, value, queue->elementSize);

	// move head index if the list was previous empty and point it at the new element we just added
	queue->headIndex = queue->headIndex < 0 ? queue->rearIndex : queue->headIndex;

	// move rear index by one, and wrap around to the front of the array if we're at the end
	queue->rearIndex = (++queue->rearIndex) >= queue->maxCapacity ? 0 : queue->rearIndex;
}

bool KIWI_QueuePop(struct KIWI_Queue* queue, void* outValue)
{
	KIWI_ASSERT(queue);

	if (queue->headIndex < 0)
	{
		return false;
	}

	void* topValue = KIWI_GetQueueElement(queue, queue->headIndex);
	memcpy(outValue, topValue, queue->elementSize);

	// move head index by one, and wrap around to the front of the array if we're at the end
	queue->headIndex = (++queue->headIndex) >= queue->maxCapacity ? 0 : queue->headIndex;

	// if the head index is now equal to the rear then the list is empty
	queue->headIndex = queue->headIndex == queue->rearIndex ? -1 : queue->headIndex;

	return true;
}
