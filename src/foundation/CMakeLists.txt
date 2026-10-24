project(foundation)

# foundation 目前只提供头文件(log.h), 因此是 INTERFACE 库:
# 不产生 .lib, 只把 include 目录和依赖(spdlog)传递给使用方
add_library(${PROJECT_NAME} INTERFACE)
add_library(vwz::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# 日志系统: 使用 spdlog(见 cmake/addspdlog.cmake)
target_link_libraries(${PROJECT_NAME} INTERFACE spdlog::spdlog)
