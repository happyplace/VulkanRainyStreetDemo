cmake_minimum_required(VERSION 3.24.0 FATAL_ERROR)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0025 NEW)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(ENABLE_IMGUI ON)

project(VulkanRainyStreetDemo)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Only 64-bit builds supported.")
endif()

add_definitions(-DNOMINMAX)

add_definitions(-DVK_DEBUG)
#add_definitions(-DVK_PORTABILITY)
if (${ENABLE_IMGUI})
    add_definitions(-DIMGUI_ENABLED)
endif()

include_directories(src)
set(vrsd-src
    "src/main.cpp"
    "src/GameWindow.h"
    "src/GameWindow.cpp"
    "src/VulkanRenderer.h"
    "src/VulkanRenderer.cpp"
    "src/VulkanFrameResources.h"
    "src/VulkanFrameResources.cpp"
    "src/Game.h"
    "src/Game.cpp"
    "src/GameMap.h"
    "src/GameMap.cpp"
    "src/GameMapRenderer.h"
    "src/GameMapRenderer.cpp"
    "src/GameFrameRender.h"
    "src/GameFrameRender.cpp"
    "src/ImGuiRenderer.h"
    "src/ImGuiRenderer.cpp"
)

include_directories("external/imgui-docking")
include_directories("external/imgui-docking/backends")
if (${ENABLE_IMGUI})
    set(vrsd-src ${vrsd-src}
        "external/imgui-docking/imgui.cpp"
        "external/imgui-docking/imgui_draw.cpp"
        "external/imgui-docking/imgui_tables.cpp"
        "external/imgui-docking/imgui_widgets.cpp"
        "external/imgui-docking/backends/imgui_impl_vulkan.cpp"
        "external/imgui-docking/backends/imgui_impl_sdl2.cpp"
    )
endif()

if (${CMAKE_C_COMPILER_ID} STREQUAL "MSVC")
    add_executable(VulkanRainyStreetDemo WIN32 ${vrsd-src})
else()
    add_executable(VulkanRainyStreetDemo ${vrsd-src})
endif()

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

find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})
target_link_libraries(VulkanRainyStreetDemo ${Vulkan_LIBRARIES})
