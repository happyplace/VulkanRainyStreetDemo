#include <stdio.h>
#include <cstdlib>

#include <array>
#include <atomic>
#include <chrono>
#include <thread>

#include <time.h>

#include "kiwi/KIWI_Scheduler.h"

#ifdef HAS_SUPPORT_DX12_IN_EXAMPLE
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#if defined(DEBUG) || defined(_DEBUG)
#include <crtdbg.h>
#endif // defined(DEBUG) || defined(_DEBUG)
#endif

#ifdef HAS_SUPPORT_DX12_IN_EXAMPLE
struct TestData
{
    int num1 = 0;
    int num2 = 0;
};

void TheSuperSuperTest(KIWI_Scheduler* scheduler, void* arg)
{
    (void)scheduler;
    TestData* data = reinterpret_cast<TestData*>(arg);
    (void)data;

#if defined(DEBUG) || defined(_DEBUG)
    // Enable the D3D12 debug layer.
    {
        Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
        D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
        debugController->EnableDebugLayer();
        printf("Directx 12 Test Passed\n");
    }
#endif // defined(DEBUG) || defined(_DEBUG)
}
#endif

struct PrintJobData
{
    int num = 0;
};

void TestPrintJob(KIWI_Scheduler* scheduler, void* arg)
{
    (void)scheduler;
    PrintJobData* data = reinterpret_cast<PrintJobData*>(arg);

    printf("TestPrintJob: %i\n", data->num);    
}

void TestJob(KIWI_Scheduler* scheduler, void* arg)
{
    (void)scheduler;
    (void)arg;
    KIWI_Job jobs[10];
    PrintJobData datas[10];

    for (int i = 0; i < 10; ++i)
    {
        datas[i].num = 9001 + i;

        jobs[i].entry = TestPrintJob;
        jobs[i].arg = &datas[i];
    }

    printf("TestJob - Start\n");
    KIWI_Counter* counter = NULL;
    KIWI_SchedulerAddJobs(jobs, 10, KIWI_JobPriority_Normal, &counter);
    KIWI_SchedulerWaitForCounterAndFree(counter, 0);
    printf("TestJob - End\n");
}

struct SeedData
{
    unsigned int seed = 0;
};

void GenerateNumbersAndPrint(KIWI_Scheduler* /*scheduler*/, void* arg)
{
    SeedData* data = reinterpret_cast<SeedData*>(arg);

    srand(data->seed);

    std::array<int, 10> numbers;
    for (int i = 0; i < 10; ++i)
    {
        numbers[i] = rand();
    }

    for (const int& num : numbers)
    {
        printf("Rand Num: %i\n", num);
    }
}

void StartJobs(KIWI_Scheduler* scheduler, void* arg)
{
    std::atomic_bool* endApp = reinterpret_cast<std::atomic_bool*>(arg);

    KIWI_Job jobs[3];

    int size = 2;

#ifdef HAS_SUPPORT_DX12_IN_EXAMPLE
    size++;

    TestData testData;
    testData.num1 = 8001;
    testData.num2 = 1337;
    
    jobs[2].entry = TheSuperSuperTest;
    jobs[2].arg = &testData;
#endif

    jobs[0].entry = TestJob;
    jobs[0].arg = NULL;

    SeedData seedData;
    seedData.seed = static_cast<int>(time(NULL));
    jobs[1].entry = GenerateNumbersAndPrint;
    jobs[1].arg = &seedData;

    KIWI_Counter* counter = NULL;
    KIWI_SchedulerAddJobs(jobs, size, KIWI_JobPriority_Normal, &counter);
    KIWI_SchedulerWaitForCounter(counter, 0);
    KIWI_SchedulerFreeCounter(counter);

    printf("StartJobs - End\n");

    endApp->store(true);
}

int main(int /*argc*/, char** /*argv*/)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) || defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif // defined(DEBUG) || defined(_DEBUG)

    KIWI_SchedulerParams params;
    KIWI_DefaultSchedulerParams(&params);

    KIWI_InitScheduler(&params);

    std::atomic_bool quitApp;
    quitApp.store(false);

    KIWI_Job startJob;
    startJob.entry = StartJobs;
    startJob.arg = &quitApp;
    KIWI_SchedulerAddJob(&startJob, KIWI_JobPriority_High, NULL);

    while (!quitApp.load())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    KIWI_FreeScheduler();

    return 0;
}
