include_guard(GLOBAL)

# 为工程内目标统一应用编译选项,保证所有模块行为一致。
# 用法:vwz_set_target_options(<target>)
function(vwz_set_target_options TARGET_NAME)
    if(NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "vwz_set_target_options: target '${TARGET_NAME}' does not exist.")
    endif()

    if(MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE
            /utf-8            # 源码与执行字符集按 UTF-8 解释(中文注释/字符串的前提)
            /W4               # 高警告等级
            /permissive-      # 关闭 MSVC 宽容模式,严格贴合标准语义
            /Zc:__cplusplus   # 让 __cplusplus 报告真实标准版本(C++20 而非 199711L)
            /Zc:preprocessor  # 符合标准的预处理器(变参宏展开更可靠)
            /EHsc             # C++ 标准异常模型
            /MP               # 多进程并行编译
        )
        target_compile_options(${TARGET_NAME} PRIVATE "$<$<CONFIG:Release>:/O2>")

        # 影响 Windows 头文件语义的宏必须传播给所有使用方(PUBLIC)
        target_compile_definitions(${TARGET_NAME} PUBLIC
            NOMINMAX           # 阻止 Windows 头定义 min/max 宏,避免与 std::min/max 冲突
            WIN32_LEAN_AND_MEAN
            UNICODE            # Win32 API 与资源宏走宽字符(W)版本
            _UNICODE           # CRT/TCHAR 走宽字符版本
        )
        target_compile_definitions(${TARGET_NAME} PRIVATE
            _CRT_SECURE_NO_WARNINGS
        )
    else()
        target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra)
    endif()
endfunction()
