#include "kiwi/KIWI_ThreadImplPthread.h"

#include <malloc.h>
#include <stdbool.h>
#include <string.h>

#include <sched.h>
#include <pthread.h>
#include <signal.h>

#include "kiwi/KIWI_FiberWorkerStorage.h"

#ifndef PTRD_ERR_HNDLR
#define PTRD_ERR_HNDLR(x) \
{ \
	int pth_result__ = (x); \
	if (pth_result__ != 0) \
	{ \
        KIWI_ASSERT(!#x); \
	} \
}
#endif // PTRD_ERR_HNDLR(x)

#define THREAD_NAME_MAX_SIZE 16
#define THREAD_NAME_BUFFER_SIZE 128

typedef struct KIWI_ThreadImpl
{
    pthread_t* workerThreadIds;
    int workerCount;
    pthread_cond_t workerCondVar;
    pthread_mutex_t workerMutex;

} KIWI_ThreadImpl;

void KIWI_ThreadImplCreateConditionalAndMutexVarialbe(KIWI_ThreadImpl* threadImpl)
{
    PTRD_ERR_HNDLR(pthread_cond_init(&threadImpl->workerCondVar, NULL));
    PTRD_ERR_HNDLR(pthread_mutex_init(&threadImpl->workerMutex, NULL));   
}

struct KIWI_ThreadImpl* KIWI_ThreadImplCreateAndStartWorkerThreads(int workerCount, void*(*threadFunction) (void*))
{
    KIWI_ThreadImpl* threadImpl = malloc(sizeof(KIWI_ThreadImpl));
    if (threadImpl == NULL)
    {
        KIWI_ASSERT(!"are we out of memory???");
        return NULL;
    }

    threadImpl->workerThreadIds = malloc(sizeof(pthread_t) * workerCount);
    if (threadImpl->workerThreadIds == NULL)
    {
        KIWI_ASSERT(!"are we out of memory???");
        return NULL;
    }

    threadImpl->workerCount = workerCount;

    KIWI_ThreadImplCreateConditionalAndMutexVarialbe(threadImpl);

    char threadNameBuffer[THREAD_NAME_BUFFER_SIZE];

    for (int i = 0; i < workerCount; ++i)
    {
        KIWI_FiberWorkerStorage* workerStorage = KIWI_GetFiberWorkerStorage(i);
        workerStorage->threadImpl = threadImpl;

        pthread_attr_t attr;
        PTRD_ERR_HNDLR(pthread_attr_init(&attr));

        cpu_set_t cpuAffinity;
        CPU_ZERO(&cpuAffinity);
        CPU_SET((size_t)i, &cpuAffinity);
        PTRD_ERR_HNDLR(pthread_attr_setaffinity_np(&attr, sizeof(cpuAffinity), &cpuAffinity));

        pthread_t threadId = 0;
        PTRD_ERR_HNDLR(pthread_create(&threadId, &attr, threadFunction, NULL));
        memcpy((char*)threadImpl->workerThreadIds + (sizeof(pthread_t) * i), &threadId, sizeof(threadId));

        int threadNameSize = snprintf(threadNameBuffer, THREAD_NAME_BUFFER_SIZE, "Kiwi Worker %i", i);
		// if the resulting thread name is going to be larger than the max thread name size, truncate the name
		if (threadNameSize > THREAD_NAME_MAX_SIZE)
		{
			threadNameBuffer[THREAD_NAME_MAX_SIZE - 1] = '\0';
		}

        PTRD_ERR_HNDLR(pthread_setname_np(threadId, threadNameBuffer));

        PTRD_ERR_HNDLR(pthread_attr_destroy(&attr));
    }

    return threadImpl;
}

void KIWI_ThreadImplShutdownWorkerThreads(struct KIWI_ThreadImpl* threadImpl)
{
    KIWI_ASSERT(threadImpl);

    for (int i = 0; i < threadImpl->workerCount; ++i)
    {
        pthread_t threadId = 0;
        memcpy(&threadId, (char*)threadImpl->workerThreadIds + (sizeof(pthread_t) * i), sizeof(pthread_t));
        PTRD_ERR_HNDLR(pthread_join(threadId, NULL));
    }

    PTRD_ERR_HNDLR(pthread_cond_destroy(&threadImpl->workerCondVar));
    PTRD_ERR_HNDLR(pthread_mutex_destroy(&threadImpl->workerMutex));

    free(threadImpl->workerThreadIds);
	free(threadImpl);
}

void KIWI_ThreadImplBlockSignalsOnWorkerThread()
{
    sigset_t mask;
    sigfillset(&mask);
    PTRD_ERR_HNDLR(pthread_sigmask(SIG_BLOCK, &mask, NULL));
}

int KIWI_ThreadImplGetCpuCount()
{
    cpu_set_t cpuset;
    PTRD_ERR_HNDLR(sched_getaffinity(0, sizeof(cpuset), &cpuset));
    return (int)CPU_COUNT(&cpuset);
}

void KIWI_ThreadImplNotifyOneWorkerThread(struct KIWI_ThreadImpl* threadImpl)
{
    KIWI_ASSERT(threadImpl);

    PTRD_ERR_HNDLR(pthread_cond_signal(&threadImpl->workerCondVar));
}

void KIWI_ThreadImplNotifyAllWorkerThreads(struct KIWI_ThreadImpl* threadImpl)
{
    KIWI_ASSERT(threadImpl);

    PTRD_ERR_HNDLR(pthread_cond_broadcast(&threadImpl->workerCondVar));
}

void KIWI_ThreadImplSleepUntilJobAdded(struct KIWI_ThreadImpl* threadImpl, atomic_bool* quitWorkerThreads)
{
    KIWI_ASSERT(threadImpl);

    PTRD_ERR_HNDLR(pthread_mutex_lock(&threadImpl->workerMutex));
    PTRD_ERR_HNDLR(pthread_cond_wait(&threadImpl->workerCondVar, &threadImpl->workerMutex));
    PTRD_ERR_HNDLR(pthread_mutex_unlock(&threadImpl->workerMutex));
}

void KIWI_ThreadImplSignalWorkerThreadsToQuit(struct KIWI_ThreadImpl* threadImpl, atomic_bool* quitWorkerThreads)
{
	KIWI_ASSERT(threadImpl);

    PTRD_ERR_HNDLR(pthread_mutex_lock(&threadImpl->workerMutex));
	atomic_store(quitWorkerThreads, true);
    PTRD_ERR_HNDLR(pthread_mutex_unlock(&threadImpl->workerMutex));
}

int KIWI_ThreadImplGetWorkerThreadIndex()
{
    pthread_t thread = pthread_self();

    cpu_set_t cpuset;
    int result = pthread_getaffinity_np(thread, sizeof(cpuset), &cpuset);
    if (result != 0)
    {
        KIWI_ASSERT(false);
        return -1;
    }

    for (int i = 0; i < CPU_SETSIZE; i++)
    {
        if (CPU_ISSET(i, &cpuset))
        {
            // assuming this is called from a worker thread, only one cpu should be set so we can return on the first true
            return i;
        }
    }

    KIWI_ASSERT(!"Why is no cpu set, is this being called outside of a scheduler thread?");
    return -1;
}
