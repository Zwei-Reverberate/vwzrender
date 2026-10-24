cmake_minimum_required(VERSION 3.25)

project(vwzrender LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 统一编译选项(/utf-8、/W4 等), 通过 vwz_set_target_options(<target>) 应用到各目标
include(${CMAKE_SOURCE_DIR}/cmake/compileroptions.cmake)

include(${CMAKE_SOURCE_DIR}/cmake/addzwindow.cmake)
vwz_add_zwindow()

include(${CMAKE_SOURCE_DIR}/cmake/addglm.cmake)
vwz_add_glm()

include(${CMAKE_SOURCE_DIR}/cmake/addspdlog.cmake)
vwz_add_spdlog()

add_subdirectory(src/foundation)
add_subdirectory(src/vwzvk)
add_subdirectory(src/main)