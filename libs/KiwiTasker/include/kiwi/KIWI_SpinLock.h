#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct KIWI_SpinLock;

// Creates a spin lock, the memory returned needs to be freed using KIWI_FreeSpinLock when done with the lock
struct KIWI_SpinLock* KIWI_CreateSpinLock();

// frees memory of a spin lock
void KIWI_FreeSpinLock(struct KIWI_SpinLock* lock);

void KIWI_LockSpinLock(struct KIWI_SpinLock* lock);

void KIWI_UnlockSpinLock(struct KIWI_SpinLock* lock);

#ifdef __cplusplus
}
#endif
