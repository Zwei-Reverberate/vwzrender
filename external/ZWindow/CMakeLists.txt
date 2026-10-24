cmake_minimum_required(VERSION 3.24)
project(TOP)

set(ZWINDOW_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

option(ZWINDOW_BUILD_EXAMPLE "Build ZWindow example" ${PROJECT_IS_TOP_LEVEL})

add_subdirectory(src/zwindow)

if(ZWINDOW_BUILD_EXAMPLE)
    add_subdirectory(example)
endif()
