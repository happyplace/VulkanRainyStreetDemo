#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct KIWI_Array;

// creates an array with a max capacity of maxCapacity and a element size of elementSize. returns NULL on error.
// The pointer returned by this function needs a call to KIWI_FreeArray to free the memory
struct KIWI_Array* KIWI_CreateArray(int elementSize, int maxCapacity);

// This frees all the memory associated with an array created by KIWI_CreateArray
void KIWI_FreeArray(struct KIWI_Array* dArray);

// get element at index
void* KIWI_ArrayGet(struct KIWI_Array* dArray, int index);

// returns true if the queue is empty
bool KIWI_ArrayIsEmpty(struct KIWI_Array* dArray);

// returns true if the queue is full
bool KIWI_ArrayIsFull(struct KIWI_Array* dArray);

int KIWI_ArraySize(struct KIWI_Array* dArray);

// add item to end of array
void KIWI_ArrayAddItem(struct KIWI_Array* dArray, void* value);

// remove item from array by swapping it with the last item
void KIWI_ArrayRemoveItem(struct KIWI_Array* dArray, int index);

#ifdef __cplusplus
}
#endif
