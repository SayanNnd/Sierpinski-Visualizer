cmake_minimum_required(VERSION 3.14)
project(Sierpinski_Visualizer CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

find_package(Threads REQUIRED)

add_executable(Sierpinski_Visualizer main.cpp)
target_link_libraries(Sierpinski_Visualizer PRIVATE raylib Threads::Threads)