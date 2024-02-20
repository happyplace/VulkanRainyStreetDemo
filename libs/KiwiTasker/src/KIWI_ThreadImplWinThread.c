#include "kiwi/KIWI_ThreadImplWinThread.h"

#include <malloc.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "kiwi/KIWI_Std.h"
#include "kiwi/KIWI_FiberWorkerStorage.h"

#define MS_VC_EXCEPTION 0x406D1388
#define MS_CORES_PER_GROUP 64
#define THREAD_NAME_MAX_SIZE 16
#define THREAD_NAME_BUFFER_SIZE 128

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // Must be 0x1000.
	LPCSTR szName; // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1=caller thread).
	DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

typedef struct KIWI_ThreadImpl
{
	HANDLE* handles;
	int workerCount;
	CONDITION_VARIABLE workerCondVar;
	CRITICAL_SECTION workerMutex;
} KIWI_ThreadImpl;

int KIWI_ThreadImplGetCpuCount()
{
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	return (int)systemInfo.dwNumberOfProcessors;
}

void KIWI_ThreadImplCreateConditionalAndMutexVarialbe(KIWI_ThreadImpl* threadImpl)
{
	InitializeConditionVariable(&threadImpl->workerCondVar);
	InitializeCriticalSection(&threadImpl->workerMutex);
}

struct KIWI_ThreadImpl* KIWI_ThreadImplCreateAndStartWorkerThreads(int workerCount, DWORD(*threadFunction) (LPVOID))
{
	KIWI_ThreadImpl* threadImpl = malloc(sizeof(KIWI_ThreadImpl));
	if (threadImpl == NULL)
	{
		KIWI_ASSERT(!"are we out of memory???");
		return NULL;
	}

	threadImpl->handles = malloc(sizeof(HANDLE) * workerCount);
	if (threadImpl->handles == NULL)
	{
		KIWI_ASSERT(!"are we out of memory???");
		free(threadImpl);		
		return NULL;
	}

	threadImpl->workerCount = workerCount;

	KIWI_ThreadImplCreateConditionalAndMutexVarialbe(threadImpl);

	char threadNameBuffer[THREAD_NAME_BUFFER_SIZE];

	for (int i = 0; i < workerCount; ++i)
	{
		KIWI_FiberWorkerStorage* workerStorage = KIWI_GetFiberWorkerStorage(i);
		workerStorage->threadImpl = threadImpl;

		DWORD threadId = 0;
		threadImpl->handles[i] = CreateThread(NULL, 0, threadFunction, NULL, CREATE_SUSPENDED, &threadId);
		if (threadImpl->handles[i] == NULL)
		{
			KIWI_ASSERT(!"why did we fail to make a thread, what do even do here????");
			exit(1);
		}		

		// when your system has more than 64 cores they're seperated into groups, this determines the group then assigns the thread to a specific
		// core within that group
		GROUP_AFFINITY groupAffinity;
		ZeroMemory(&groupAffinity, sizeof(GROUP_AFFINITY));
		groupAffinity.Mask = (DWORD_PTR)1 << (i % MS_CORES_PER_GROUP);
		groupAffinity.Group = (WORD)(i / MS_CORES_PER_GROUP);
		bool result = SetThreadGroupAffinity(threadImpl->handles[i], &groupAffinity, NULL);
		if (result == false)
		{
			KIWI_ASSERT(!"why did we fail to change the affinity, what do even do here????");
			exit(1);
		}

		ResumeThread(threadImpl->handles[i]);

		int threadNameSize = snprintf(threadNameBuffer, THREAD_NAME_BUFFER_SIZE, "Kiwi Worker %i", i);
		// if the resulting thread name is going to be larger than the max thread name size, truncate the name
		if (threadNameSize > THREAD_NAME_MAX_SIZE)
		{
			threadNameBuffer[THREAD_NAME_MAX_SIZE - 1] = '\0';
		}

		THREADNAME_INFO info;
		info.dwType = 0x1000;
		info.szName = threadNameBuffer;
		info.dwThreadID = threadId;
		info.dwFlags = 0;

		__try
		{
			RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
		}		
	}
	
	return threadImpl;
}

void KIWI_ThreadImplShutdownWorkerThreads(struct KIWI_ThreadImpl* threadImpl)
{
	KIWI_ASSERT(threadImpl);

	WaitForMultipleObjects(threadImpl->workerCount, threadImpl->handles, TRUE, INFINITE);

	for (int i = 0; i < threadImpl->workerCount; ++i)
	{
		CloseHandle(threadImpl->handles[i]);
	}

	free(threadImpl->handles);
	free(threadImpl);
}

void KIWI_ThreadImplBlockSignalsOnWorkerThread()
{
	// from my research this isn't something that happens on windows so we do nothing here
}

int KIWI_ThreadImplGetWorkerThreadIndex()
{
	GROUP_AFFINITY groupAffinity;
	ZeroMemory(&groupAffinity, sizeof(GROUP_AFFINITY));
	bool result = GetThreadGroupAffinity(GetCurrentThread(), &groupAffinity);
	if (result == false)
	{
		KIWI_ASSERT(!"why did we fail to get the affinity???");
		return -1;
	}

	const int threadIndex = (int)(groupAffinity.Group * MS_CORES_PER_GROUP);
	for (int i = 0; i < MS_CORES_PER_GROUP; ++i)
	{
		const DWORD_PTR mask = (DWORD_PTR)(1) << i;
		// we assume this will only be called by worker threads so when we find the first set affinity
		// we add it to the threadIndex and return that value
		if ((groupAffinity.Mask & mask) != 0)
		{
			return threadIndex + i;
		}
	}

	KIWI_ASSERT(!"Why is no cpu set, is this being called outside of a scheduler thread?");
	return -1;
}


void KIWI_ThreadImplNotifyOneWorkerThread(struct KIWI_ThreadImpl* threadImpl)
{
	KIWI_ASSERT(threadImpl);

	WakeConditionVariable(&threadImpl->workerCondVar);
}

void KIWI_ThreadImplNotifyAllWorkerThreads(struct KIWI_ThreadImpl* threadImpl)
{
	KIWI_ASSERT(threadImpl);

	WakeAllConditionVariable(&threadImpl->workerCondVar);
}

void KIWI_ThreadImplSleepUntilJobAdded(struct KIWI_ThreadImpl* threadImpl, atomic_bool* quitWorkerThreads)
{
	KIWI_ASSERT(threadImpl);

	EnterCriticalSection(&threadImpl->workerMutex);
	if (!atomic_load(quitWorkerThreads))
	{
		SleepConditionVariableCS(&threadImpl->workerCondVar, &threadImpl->workerMutex, INFINITE);
	}	
	LeaveCriticalSection(&threadImpl->workerMutex);
}

void KIWI_ThreadImplSignalWorkerThreadsToQuit(struct KIWI_ThreadImpl* threadImpl, atomic_bool* quitWorkerThreads)
{
	KIWI_ASSERT(threadImpl);

	EnterCriticalSection(&threadImpl->workerMutex);
	atomic_store(quitWorkerThreads, true);
	LeaveCriticalSection(&threadImpl->workerMutex);
}
