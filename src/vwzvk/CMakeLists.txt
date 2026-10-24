project(vwzvk)

file(GLOB_RECURSE VWZVK_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
file(GLOB_RECURSE VWZVK_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)

add_library(${PROJECT_NAME} STATIC ${VWZVK_SOURCES} ${VWZVK_HEADERS})
add_library(vwz::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(${PROJECT_NAME} PUBLIC vwz::foundation)

include(${CMAKE_SOURCE_DIR}/cmake/AddVulkan.cmake)
link_vulkan(${PROJECT_NAME})

vwz_set_target_options(${PROJECT_NAME})