include_guard(GLOBAL)

find_package(Vulkan REQUIRED)
if(NOT TARGET Vulkan::Vulkan)
    message(FATAL_ERROR "Vulkan::Vulkan target not found. Is the Vulkan SDK installed and VULKAN_SDK set?")
endif()

function(link_vulkan TARGET_NAME)
    if(NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "link_vulkan: target '${TARGET_NAME}' does not exist.")
    endif()

    target_link_libraries(${TARGET_NAME} PUBLIC Vulkan::Vulkan)
    target_compile_definitions(${TARGET_NAME} PUBLIC VK_USE_PLATFORM_WIN32_KHR)
endfunction()
