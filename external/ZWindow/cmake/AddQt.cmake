include_guard(GLOBAL)

function(zwin_detect_qt)
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

function(zwin_link_qt TARGET_NAME)
    if(NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "zwin_link_qt: target '${TARGET_NAME}' does not exist.")
    endif()

    set_target_properties(${TARGET_NAME} PROPERTIES
        AUTOMOC ON
    )

    # PRIVATE:zwindow.h 不暴露任何 Qt 类型,Qt 只是实现细节。
    # 用 PUBLIC 会强迫每个消费方都找到同一个 Qt、并继承 Qt 的 include 路径。
    # 静态库的 PRIVATE 依赖在最终链接时仍会传递给可执行文件(CMake 以
    # $<LINK_ONLY:...> 的形式保留),所以这里的 PRIVATE 不会漏链接。
    target_link_libraries(${TARGET_NAME} PRIVATE
        Qt${ZWIN_QT_VERSION_MAJOR}::Core
        Qt${ZWIN_QT_VERSION_MAJOR}::Gui
    )

endfunction()
