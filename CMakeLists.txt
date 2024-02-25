cmake_minimum_required(VERSION 3.24.0 FATAL_ERROR)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0025 NEW)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(VulkanRainyStreetDemo)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Only 64-bit builds supported.")
endif()

add_definitions(-DNOMINMAX)

include_directories(src)
set(vrsd-src
    "src/main.cpp"
    "src/Window.h"
    "src/Window.cpp"
    "src/EntryJob.h"
    "src/EntryJob.cpp"
    "src/Singleton.h"
)

if (${CMAKE_C_COMPILER_ID} STREQUAL "MSVC")
    add_executable(VulkanRainyStreetDemo WIN32 ${vrsd-src})
else()
    add_executable(VulkanRainyStreetDemo ${vrsd-src})
endif()

target_compile_features(VulkanRainyStreetDemo PRIVATE cxx_std_14)
#set_property(TARGET VulkanRainyStreetDemo PROPERTY COMPILE_WARNING_AS_ERROR ON)

if (${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(VulkanRainyStreetDemo PRIVATE -Wall)
elseif(${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
    target_compile_options(VulkanRainyStreetDemo PRIVATE -Wall -Wextra)
elseif (${CMAKE_C_COMPILER_ID} STREQUAL "MSVC")
    target_compile_options(VulkanRainyStreetDemo PRIVATE /nologo /W4 /MP /GL /EHs)
endif()

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})
target_link_libraries(VulkanRainyStreetDemo ${SDL2_LIBRARY} ${SDL2MAIN_LIBRARY})

add_subdirectory("libs/KiwiTasker")
include_directories("libs/KiwiTasker/include")
target_link_libraries(VulkanRainyStreetDemo KiwiTasker)

