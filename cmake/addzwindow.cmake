include_guard(GLOBAL)

function(vwz_find_qt)
	if(DEFINED ENV{QTDIR} AND NOT CMAKE_PREFIX_PATH)
        set(CMAKE_PREFIX_PATH "$ENV{QTDIR}")
    endif()

    find_package(QT NAMES Qt6 Qt5 QUIET COMPONENTS Core Gui)
    if(QT_VERSION_MAJOR)
        find_package(Qt${QT_VERSION_MAJOR} QUIET COMPONENTS Core Gui)
    endif()
    
    if(QT_VERSION_MAJOR AND TARGET Qt${QT_VERSION_MAJOR}::Core AND TARGET Qt${QT_VERSION_MAJOR}::Gui)
        set(ZWIN_QT_FOUND TRUE PARENT_SCOPE)
        set(ZWIN_QT_VERSION_MAJOR "${QT_VERSION_MAJOR}" PARENT_SCOPE)
        set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" PARENT_SCOPE)
        message(STATUS "Found Qt${QT_VERSION_MAJOR} (Core, Gui)")
    else()
        set(ZWIN_QT_FOUND FALSE PARENT_SCOPE)
        message(STATUS "Qt not found")
    endif()
endfunction()

function(vwz_add_zwindow)
    if(TARGET zwindow)
        return()
    endif()

    set(VWZ_ZWINDOW_DIR "${CMAKE_SOURCE_DIR}/external/ZWindow")
    set(ZWIN_QT_FOUND FALSE)
    set(ZWINDOW_BUILD_EXAMPLE OFF CACHE BOOL "not build zwindow example" FORCE)

    vwz_find_qt()

    if(ZWIN_QT_FOUND)
        set(ZWINDOW_BACKEND "Qt" CACHE STRING "zwindow use qt backend" FORCE)
    else()
         set(ZWINDOW_BACKEND "Win32" CACHE STRING "zwindow use win32 backend" FORCE)
    endif()
    set_property(CACHE ZWINDOW_BACKEND PROPERTY STRINGS Auto Qt Win32)

    if(EXISTS "${VWZ_ZWINDOW_DIR}/CMakeLists.txt")
        message(STATUS "zwindow source code already exists")
        add_subdirectory("${VWZ_ZWINDOW_DIR}" "${CMAKE_BINARY_DIR}/_deps/zwindow-build")
    else()
        message(STATUS "fetch zwindow source code")
        include(FetchContent)
        FetchContent_Declare(zwindow
            GIT_REPOSITORY https://github.com/Zwei-Reverberate/ZWindow.git
            GIT_TAG        f91e9cd6295a2a1e64f4161bcc0a63ded957514d
            SOURCE_DIR     "${VWZ_ZWINDOW_DIR}"
            UPDATE_DISCONNECTED TRUE
        )
        FetchContent_MakeAvailable(zwindow)
    endif()

    if(NOT TARGET zwindow)
        message(FATAL_ERROR "zwindow add failed")
    endif()

    if(MSVC)
        target_compile_options(zwindow PRIVATE /utf-8)
    endif()
endfunction()
