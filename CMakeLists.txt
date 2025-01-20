cmake_minimum_required(VERSION 3.10)
project(WizCam)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_library(shared STATIC
    shared/src/processutils.cpp
)
target_include_directories(shared PUBLIC shared/include)

add_executable(zoomhack
    zoomhack/src/zoomhack.cpp
)
target_link_libraries(zoomhack PRIVATE shared)

add_executable(freecam
    freecam/src/main.cpp
    freecam/src/camera.cpp
    freecam/src/patcher.cpp
)

target_include_directories(freecam PRIVATE freecam/include)