include_guard(GLOBAL)

function(vwz_add_spdlog)
    if(TARGET spdlog::spdlog)
        return()
    endif()

    set(VWZ_SPDLOG_DIR "${CMAKE_SOURCE_DIR}/external/spdlog")

    set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "not build spdlog example" FORCE)
    set(SPDLOG_BUILD_EXAMPLE_HO OFF CACHE BOOL "not build spdlog header-only example" FORCE)
    set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "not build spdlog tests" FORCE)
    set(SPDLOG_BUILD_TESTS_HO OFF CACHE BOOL "not build spdlog header-only tests" FORCE)
    set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "not build spdlog benchmarks" FORCE)
    set(SPDLOG_INSTALL OFF CACHE BOOL "not generate spdlog install rules" FORCE)

    if(EXISTS "${VWZ_SPDLOG_DIR}/include/spdlog/spdlog.h")
        message(STATUS "spdlog source code already exists")
        add_subdirectory("${VWZ_SPDLOG_DIR}" "${CMAKE_BINARY_DIR}/_deps/spdlog-build")
    else()
        message(STATUS "fetch spdlog source code")
        include(FetchContent)
        FetchContent_Declare(spdlog
            GIT_REPOSITORY https://github.com/gabime/spdlog.git
            GIT_TAG        6fa36017cfd5731d617e1a934f0e5ea9c4445b13
            SOURCE_DIR     "${VWZ_SPDLOG_DIR}"
            UPDATE_DISCONNECTED TRUE
        )
        FetchContent_MakeAvailable(spdlog)
    endif()

    if(NOT TARGET spdlog::spdlog)
        message(FATAL_ERROR "spdlog add failed")
    endif()
endfunction()
