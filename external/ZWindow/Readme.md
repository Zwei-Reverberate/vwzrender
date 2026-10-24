# ZWindow

跨后端(Win32 / Qt `QWindow`)的极简窗口与输入抽象库。对外只暴露一个不依赖任何平台头文件的接口
`zwindow.h`,后端在 CMake 配置期二选一,产物里不含另一后端的代码。

## 特性

- 单头文件接口:`#include <zwindow.h>`,不含 Qt / Windows 类型
- 两个后端:Win32 原生、Qt(`QWindow`)
- 后端在配置期选定,互斥编译
- 两个后端统一的**物理像素**尺寸与坐标语义
- 键盘、文本输入、鼠标按键/移动/滚轮、尺寸变化、关闭事件回调

## 系统要求

| 项 | 要求 |
| --- | --- |
| CMake | ≥ 3.24 |
| 编译器 | 支持 C++20(库以 `target_compile_features(... PUBLIC cxx_std_20)` 声明) |
| Win32 后端 | Windows 10 1703(创意者更新)及以上 |
| Qt 后端 | Qt 5.15 或 Qt 6.x,需要 `Core` + `Gui` 组件 |

需要注意的两点:

- Win32 后端用到了 `SetProcessDpiAwarenessContext` 与 `AdjustWindowRectExForDpi`,所以运行期最低是
  Windows 10 1703。老系统上无法加载。
- Win32 后端会在首次使用时把**整个进程**设置为 Per-Monitor V2 DPI 感知。这是进程级副作用:宿主如果
  有自己的 DPI 策略,请在构造 `Application` 之前设置好;库检测到已设置时会保留既有设置,只在 stderr
  提示一句。
- Qt 后端需要一个可用的屏幕,无头会话下 `ZWindow::Create()` 会返回 `nullptr`。

## 接入方式

目标名固定为 `zwindow`。消费方不需要做任何 Qt / Windows 相关配置 —— Qt 是库的 **PRIVATE** 依赖,
不会污染你的 include 路径,也不要求你链接 Qt(静态库的 PRIVATE 依赖在最终链接时由 CMake 自动传递)。

### FetchContent(推荐)

```cmake
include(FetchContent)

FetchContent_Declare(zwindow
    GIT_REPOSITORY https://github.com/Zwei-Reverberate/ZWindow.git
    GIT_TAG        <commit 或 tag>   # 建议用 tag,避免上游改动破坏你的构建
    GIT_SHALLOW    TRUE)

FetchContent_MakeAvailable(zwindow)

target_link_libraries(<你的目标> PRIVATE zwindow)
```

被引入时 `ZWINDOW_BUILD_EXAMPLE` 默认为 `OFF`,不会连带构建示例。

### git 子模块

```bash
git submodule add https://github.com/Zwei-Reverberate/ZWindow.git third_party/ZWindow
```

```cmake
add_subdirectory(third_party/ZWindow)
target_link_libraries(<你的目标> PRIVATE zwindow)
```

### 本地路径

```cmake
add_subdirectory(<ZWindow 路径>)
target_link_libraries(<你的目标> PRIVATE zwindow)
```

```cpp
#include <zwindow.h>
```

### CMake 选项

| 选项 | 取值 | 说明 |
| --- | --- | --- |
| `ZWINDOW_BACKEND` | `Auto`(默认) / `Qt` / `Win32` | `Auto`:检测到 Qt(优先 Qt6,回退 Qt5)就用 Qt 后端,否则用 Win32。`Qt`:强制 Qt,找不到 Qt 直接配置报错。`Win32`:强制 Win32,非 Windows 平台上报配置错误。 |
| `ZWINDOW_BUILD_EXAMPLE` | `ON` / `OFF` | 是否构建示例程序。作为顶层工程单独配置时默认 `ON`,被 FetchContent / 子模块引入时默认 `OFF`。 |

强制后端的方式:

```
cmake -DZWINDOW_BACKEND=Win32 ..
```

### 可选:复用 Qt 辅助函数

`cmake/AddQt.cmake` 里的两个函数 `include` 之后即可使用(库自身也是这么用的):

| 函数 | 作用 |
| --- | --- |
| `zwin_detect_qt()` | 静默探测 Qt(优先 Qt6,回退 Qt5),在父作用域设置 `ZWIN_QT_FOUND` 与 `ZWIN_QT_VERSION_MAJOR` |
| `zwin_link_qt(<target>)` | 为目标开启 `AUTOMOC`,并以 PRIVATE 链接 `Qt<major>::Core` + `Qt<major>::Gui`。调用前需要先执行 `zwin_detect_qt()` |

## 快速开始

```cpp
#include <zwindow.h>

int main(int argc, char** argv)
{
    zwin::Application app(argc, argv);   // 必须最先构造、最后析构

    auto window = zwin::ZWindow::Create({ .title = "Hello", .width = 1280, .height = 720 });
    if (window == nullptr)               // 后端未能建立原生窗口
    {
        return 1;
    }

    window->onKey = [&window](zwin::Key key, bool down, bool /*repeat*/, zwin::KeyMod /*mods*/)
    {
        if (down && key == zwin::Key::Escape)
        {
            window->requestClose();
        }
    };

    if (!window->show())
    {
        return 1;
    }

    while (window->processEvents())      // 非阻塞抽干事件;无渲染需求时可改用 waitEvents()
    {
        // 渲染 / 更新
    }
    return 0;
}
```

## 契约

### 尺寸与坐标

- `WindowConfig::width/height` 指**客户区**的物理像素数,不含边框与标题栏。
- `clientWidth()` / `clientHeight()`、`onResize` 的参数、`onMouseMove` 的坐标一律是**物理像素**,
  即已经乘过 DPI 缩放比;含义不随显示器 DPI 变化。两个后端完全一致。
- `show()` 之前 `clientWidth()` / `clientHeight()` 返回 `WindowConfig` 里的请求值。
- 全屏时 `WindowConfig::width/height` 被忽略:Win32 后端用无边框窗口覆盖主显示器,Qt 后端走
  `showFullScreen()`。
- 窗口最小化时客户区是 `0 x 0`,`onResize(0, 0)` 会照实上报,渲染端必须显式处理。

### 生命周期与线程

- `Application` 必须**最先构造、最后析构**。Qt 后端下 `QGuiApplication` 必须比所有 `QWindow` 长寿;
  Win32 后端没有这个约束(不构造 `Application` 也能创建窗口),但请统一写法,以免换后端时出现难以
  定位的崩溃。
- 所有方法调用与回调都必须发生在创建窗口的那个线程上(两后端的消息/事件队列都是线程私有的)。
- **不要在回调内部销毁 `ZWindow` 或 `Application` 对象。**
- 关闭是延迟的:`requestClose()` 只置标记并触发 `onClose`,原生窗口留到析构时才真正销毁。

### 返回值

- `ZWindow::Create()` 返回 `nullptr` 表示后端无法建立原生窗口。
- `show()` 返回 `false` 表示窗口没有有效的原生窗口,或此前已经请求过关闭。
- `processEvents()` 的返回值等价于 `isOpen()`。

## API 概览

方法:
`Create`、`backendName`、`show`、`requestClose`、`isOpen`、`processEvents`、`waitEvents`、`setTitle`、
`nativeWindow`、`clientWidth`、`clientHeight`

回调(全部是 `std::function`,在窗口线程上触发):

| 回调 | 签名 | 触发时机 |
| --- | --- | --- |
| `onClose` | `void()` | 关闭按钮或 `requestClose()` |
| `onKey` | `void(Key, bool down, bool repeat, KeyMod)` | 按键按下/抬起,`repeat` 表示自动重复 |
| `onChar` | `void(char32_t)` | 文本输入,逐个完整码点(含辅助平面字符) |
| `onResize` | `void(uint32_t w, uint32_t h)` | 客户区尺寸变化,物理像素 |
| `onMouseMove` | `void(int32_t x, int32_t y, KeyMod)` | 客户区坐标,物理像素 |
| `onMouseButton` | `void(MouseButton, bool down, KeyMod)` | 鼠标按键按下/抬起 |
| `onMouseWheel` | `void(int32_t dx, int32_t dy)` | 滚轮,单位 120/格,正为向上 / 向右 |

## 构建示例

```bash
cmake -B build -DBUILD_EXAMPLE=ON -DCMAKE_PREFIX_PATH=C:/Qt/6.9.3/msvc2022_64
cmake --build build --config Debug

./build/example/Debug/example.exe             # 交互运行:ESC 或关闭按钮退出
./build/example/Debug/example.exe --smoke 60  # 冒烟:跑 60 帧后自动退出
```

## 目录结构

```
CMakeLists.txt              顶层接入点,只负责 add_subdirectory
cmake/AddQt.cmake           Qt 探测与链接辅助
src/zwindow/
  CMakeLists.txt            后端选择与目标 zwindow
  include/zwindow.h         唯一对外头文件
  src/zwindow.cpp           后端无关的工厂
  src/zwindowqt.cpp         Qt 后端
  src/zwindowwin32.cpp      Win32 后端
example/                    示例程序
```

## 已知限制

- 没有运行时全屏切换、光标控制、窗口位置设置、焦点事件、鼠标离开事件、DPI 变化回调。
- 未处理输入法(IME):Win32 后端未接 `WM_IME_*`,Qt 后端未接 `QEvent::InputMethod`,中文/日文输入法
  的候选窗与组合文本行为未定义。
- `onChar` 的跨后端语义(尤其是控制字符与 Ctrl 组合键)尚未被测试固定,两个后端可能存在差异。
- 尚未提供自动化测试。
- 同一构建内无法让不同目标使用不同后端(`ZWINDOW_BACKEND` 是全局 cache 变量)。

## 许可证

[MIT](LICENSE)
