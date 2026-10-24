include_guard(GLOBAL)

function(vwz_add_glm)
    if(TARGET glm::glm)
        return()
    endif()

    set(VWZ_GLM_DIR "${CMAKE_SOURCE_DIR}/external/glm")
    set(GLM_BUILD_LIBRARY OFF CACHE BOOL "not build glm library" FORCE)
    set(GLM_BUILD_TESTS OFF CACHE BOOL "not build glm tests" FORCE)
    set(GLM_BUILD_INSTALL OFF CACHE BOOL "not generate glm install rules" FORCE)

    if(EXISTS "${VWZ_GLM_DIR}/glm/glm.hpp")
        message(STATUS "glm source code already exists")
        add_subdirectory("${VWZ_GLM_DIR}" "${CMAKE_BINARY_DIR}/_deps/glm-build")
    else()
        message(STATUS "fetch glm source code")
        include(FetchContent)
        FetchContent_Declare(glm
            GIT_REPOSITORY https://github.com/g-truc/glm.git
            GIT_TAG        6f14f4792a0cde5d0cf2c910506724d61cb95834
            SOURCE_DIR     "${VWZ_GLM_DIR}"
            UPDATE_DISCONNECTED TRUE
        )
        FetchContent_MakeAvailable(glm)
    endif()

    if(NOT TARGET glm::glm)
        message(FATAL_ERROR "glm add failed")
    endif()

    # Vulkan 的规范化设备坐标深度范围为 [0, 1](OpenGL 传统为 [-1, 1])
    # 该宏让 glm::perspective / glm::ortho 直接产出 Vulkan 风格深度
    # 挂在 INTERFACE 目标上,所有链接 glm::glm 的翻译单元看到同一套数学约定(ODR 安全)
    target_compile_definitions(glm INTERFACE GLM_FORCE_DEPTH_ZERO_TO_ONE)
endfunction()
