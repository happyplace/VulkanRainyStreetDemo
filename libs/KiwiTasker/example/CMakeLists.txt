cmake_minimum_required(VERSION 3.0)

cmake_policy(SET CMP0079 NEW)
cmake_policy(SET CMP0054 NEW)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

project(kiwi-example)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "-Wall -Werror")
endif()

if (MSVC)
    set(MSVC_COMMON_FLAGS 
	    /nologo     # Don't show version info
	    /W4         # Enable warning level 4
	    /MP         # Build with multiple processes
	    /WX         # Treat warnings as errors
    )
    string(REPLACE ";" " " MSVC_COMMON_FLAGS "${MSVC_COMMON_FLAGS}")
    set(CMAKE_CXX_FLAGS "${MSVC_COMMON_FLAGS}" CACHE STRING "C++ Common Flags" FORCE)
endif()

include_directories("../include")
add_subdirectory(.. build EXCLUDE_FROM_ALL)

add_executable(kiwi-example main.cpp)

if (MSVC)
    add_definitions(-DHAS_SUPPORT_DX12_IN_EXAMPLE)
    target_link_libraries(kiwi-example d3dcompiler.lib)
    target_link_libraries(kiwi-example D3D12.lib)
    target_link_libraries(kiwi-example dxgi.lib)
endif()

target_link_libraries(kiwi-example KiwiTasker)
