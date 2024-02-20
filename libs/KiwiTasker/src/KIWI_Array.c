#include "kiwi/KIWI_Array.h"

#include <malloc.h>
#include <string.h>

#include "kiwi/KIWI_Std.h"

typedef struct KIWI_Array
{
	void* data;
	int elementSize;
	int maxCapacity;
	int size;
} KIWI_Array;

struct KIWI_Array* KIWI_CreateArray(int elementSize, int maxCapacity)
{
	KIWI_ASSERT(elementSize >= 1);
	KIWI_ASSERT(maxCapacity >= 1);

	KIWI_Array* dArray = malloc(sizeof(KIWI_Array));
	if (dArray == NULL)
	{
		KIWI_ASSERT(!"are we out of memory???");
		return NULL;
	}

	dArray->elementSize = elementSize;
	dArray->maxCapacity = maxCapacity;
	dArray->size = 0;

	dArray->data = malloc((size_t)dArray->elementSize * dArray->maxCapacity);
	if (dArray->data == NULL)
	{
		KIWI_ASSERT(!"are we out of memory???");
		return NULL;
	}

	return dArray;
}

void KIWI_FreeArray(struct KIWI_Array* dArray)
{
	KIWI_ASSERT(dArray);

	free(dArray->data);
	free(dArray);
}

void* KIWI_ArrayGet(struct KIWI_Array* dArray, int index)
{
	KIWI_ASSERT(dArray);
	KIWI_ASSERT(index >= 0 && "invalid index");

	if (index < 0)
	{
		return NULL;
	}

	return (char*)dArray->data + ((size_t)dArray->elementSize * index);
}

bool KIWI_ArrayIsEmpty(struct KIWI_Array* dArray)
{
	KIWI_ASSERT(dArray);

	return dArray->size <= 0;
}

bool KIWI_ArrayIsFull(struct KIWI_Array* dArray)
{
	KIWI_ASSERT(dArray);

	return dArray->size >= dArray->maxCapacity;
}

void KIWI_ArrayAddItem(struct KIWI_Array* dArray, void* value)
{
	KIWI_ASSERT(dArray);
	KIWI_ASSERT(!KIWI_ArrayIsFull(dArray));
	
	void* newElement = KIWI_ArrayGet(dArray, dArray->size);
	memcpy(newElement, value, dArray->elementSize);

	dArray->size++;
}

void KIWI_ArrayRemoveItem(struct KIWI_Array* dArray, int index)
{
	KIWI_ASSERT(dArray);

	if (dArray->size <= 1)
	{
		dArray->size = 0;
		return;
	}

	void* item = KIWI_ArrayGet(dArray, index);
	void* lastItem = KIWI_ArrayGet(dArray, dArray->size - 1);
	memcpy(item, lastItem, dArray->elementSize);

	dArray->size--;
}

int KIWI_ArraySize(struct KIWI_Array* dArray)
{
	KIWI_ASSERT(dArray);

	return dArray->size;
}
