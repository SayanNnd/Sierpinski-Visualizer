cmake_minimum_required(VERSION 3.14)
project(Sierpinski_Visualizer CXX)
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
)
FetchContent_MakeAvailable(raylib)

add_executable(Sierpinski_Visualizer main.cpp)
target_link_libraries(Sierpinski_Visualizer raylib)