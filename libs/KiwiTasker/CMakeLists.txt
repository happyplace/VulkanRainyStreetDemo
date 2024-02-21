cmake_minimum_required(VERSION 3.0)

cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0025 NEW)

project(KiwiTasker C)

option(ValgrindSupport "This will register fiber stacks with valgrind so it can probably track them" OFF)

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_C_FLAGS "-Wall -Werror")
endif()

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
    set(MSVC_COMMON_FLAGS 
	    /nologo     # Don't show version info
	    /W4         # Enable warning level 4
	    /MP         # Build with multiple processes
	    /WX         # Treat warnings as errors
    )
    string(REPLACE ";" " " MSVC_COMMON_FLAGS "${MSVC_COMMON_FLAGS}")
    set(CMAKE_C_FLAGS "${MSVC_COMMON_FLAGS}" CACHE STRING "C Common Flags" FORCE)
endif()

include_directories("include")
include_directories("src")

add_definitions(-DKIWI_DLL_EXPORT)

set(kiwi-tasker-src
	"include/kiwi/KIWI_Std.h"
	"include/kiwi/KIWI_Atomics.h"
	"include/kiwi/KIWI_Scheduler.h"
	"include/kiwi/KIWI_Queue.h"
	"include/kiwi/KIWI_Job.h"
	"include/kiwi/KIWI_ThreadImpl.h"
	"include/kiwi/KIWI_FiberWorkerStorage.h"
	"include/kiwi/KIWI_SpinLock.h"
	"include/kiwi/KIWI_JobPriority.h"
	"include/kiwi/KIWI_FiberPool.h"
	"include/kiwi/KIWI_Counter.h"
	"include/kiwi/KIWI_Array.h"
	"include/kiwi/KIWI_CounterPool.h"
)

set(kiwi-tasker-src ${kiwi-tasker-src}
	"src/KIWI_Scheduler.c"
	"src/KIWI_Queue.c"
	"src/KIWI_FiberWorkerStorage.c"
	"src/KIWI_SpinLock.c"
	"src/KIWI_FiberPool.c"
	"src/KIWI_Counter.c"
	"src/KIWI_Array.c"
	"src/KIWI_CounterPool.c"
)

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
	add_definitions(-DKIWI_USE_WINTHREAD_IMPL)	
	set(kiwi-tasker-src ${kiwi-tasker-src}
		"include/kiwi/KIWI_ThreadImplWinThread.h"
		"src/KIWI_ThreadImplWinThread.c"
	)
endif()

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")	
	add_definitions(-D_GNU_SOURCE)
	add_definitions(-DKIWI_USE_PTHREAD_IMPL)
	set(kiwi-tasker-src ${kiwi-tasker-src}
		"include/kiwi/KIWI_ThreadImplPthread.h"
		"src/KIWI_ThreadImplPthread.c"
	)
endif()

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
	add_definitions(-DKIWI_USE_SIMPLE_STDATOMIC)
	set(kiwi-tasker-src ${kiwi-tasker-src}
		"external/simple-stdatomic-for-VS-Clang/stdatomic.h"
		"external/simple-stdatomic-for-VS-Clang/stdatomic.c"
	)
endif()

if (ValgrindSupport)
	add_definitions(-DKIWI_ENABLE_VALGRIND_SUPPORT)
endif()

include_directories("external/deboost.context/include")
add_subdirectory("external/deboost.context")

find_package (Threads REQUIRED)

add_library(KiwiTasker ${kiwi-tasker-src})

set_property(TARGET KiwiTasker PROPERTY C_STANDARD 11)
set_property(TARGET KiwiTasker PROPERTY C_STANDARD_REQUIRED ON)

target_link_libraries(KiwiTasker Threads::Threads)
target_link_libraries(KiwiTasker fcontext)
