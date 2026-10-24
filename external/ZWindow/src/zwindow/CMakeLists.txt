project(zwindow)

set(ZWINDOW_BACKEND "Auto" CACHE STRING "ZWindow Backend:  Auto | Qt | Win32")
set_property(CACHE ZWINDOW_BACKEND PROPERTY STRINGS Auto Qt Win32)

if(NOT ZWINDOW_BACKEND MATCHES "^(Auto|Qt|Win32)$")
    message(FATAL_ERROR "Invalid ZWINDOW_BACKEND: ${ZWINDOW_BACKEND} (expected Auto | Qt | Win32)")
endif()

set(ZWINDOW_HEADERS include/zwindow.h)
set(ZWINDOW_SOURCES src/zwindow.cpp)

if(NOT DEFINED ZWINDOW_ROOT_DIR)
    message(FATAL_ERROR "ZWINDOW_ROOT_DIR is not defined.")
endif()
include("${ZWINDOW_ROOT_DIR}/cmake/AddQt.cmake")

set(ZWIN_QT_FOUND FALSE)
if(ZWINDOW_BACKEND STREQUAL "Auto")
    zwin_detect_qt()
    if(ZWIN_QT_FOUND)
        set(ZWINDOW_BACKEND Qt)
    else()
        set(ZWINDOW_BACKEND Win32)
    endif()
elseif(ZWINDOW_BACKEND STREQUAL "Qt")
    zwin_detect_qt()
    if(NOT ZWIN_QT_FOUND)
        message(FATAL_ERROR
            "ZWINDOW_BACKEND=Qt was forced, but Qt was not found. "
            "Point CMAKE_PREFIX_PATH at your Qt installation (e.g. -DCMAKE_PREFIX_PATH=C:/Qt/6.7.0/msvc2019_64), "
            "or use -DZWINDOW_BACKEND=Win32.")
    endif()
endif()

if(ZWINDOW_BACKEND STREQUAL "Qt")
    list(APPEND ZWINDOW_SOURCES src/zwindowqt.cpp)
    message(STATUS "ZWindow backend: Qt${ZWIN_QT_VERSION_MAJOR} (QWindow)")
elseif(WIN32)
    list(APPEND ZWINDOW_SOURCES src/zwindowwin32.cpp)
    message(STATUS "ZWindow backend: Win32")
else()
    message(FATAL_ERROR "ZWindow Win32 backend requires Windows; on other platforms use -DZWINDOW_BACKEND=Qt.")
endif()

add_library(${PROJECT_NAME} STATIC ${ZWINDOW_SOURCES} ${ZWINDOW_HEADERS})
add_library(zwin::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
if(ZWINDOW_BACKEND STREQUAL "Qt")
    target_compile_definitions(${PROJECT_NAME} PRIVATE ZWINDOW_BACKEND_QT)
    zwin_link_qt(${PROJECT_NAME})
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        ZWINDOW_BACKEND_WIN32
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        UNICODE
        _UNICODE
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE user32 gdi32)
endif()
