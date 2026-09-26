# winwrap

Winwrap is a modern C++ library for developers who have already chosen native Windows APIs.
It abstracts native window operations, controls, menus and tray protocols into
an ergonomic C++ API, with ownership helpers and composable message handling.
Borrowed native handles remain available for unsupported operations and integration.
It is not a cross-platform GUI framework.

> **Status:** experimental v0.1 work in progress, not yet production-ready.
> Compiled static library exposing `winwrap::winwrap`. Known lifetime and API
> contract issues are tracked in [TECH_DEBT.md](TECH_DEBT.md).

## Why

Winwrap began by removing repeated Win32 ceremony from small native applications.
Its closest architectural neighbors include WTL, Win32++ and WinLamb, rather than
only higher-level GUI toolkits. The goal is a coherent native desktop façade—not
a claim that this architectural layer or integrated tray support is unique.
The source-based [technical and product assessment](ASSESSMENT.md) compares those
alternatives, records limitations, and proposes an application-driven direction.
Learning modern C++ through useful native applications is a first-class purpose;
the [vision](VISION.md#why-it-exists) separates that value from public adoption.

- **Compile-time message composition.** No vtables, no virtual hierarchy, no macro
  message maps. Messages route to named `on_*` methods your window defines, detected
  with `requires` and resolved by `if constexpr`, with the final type deduced via
  C++23 *deducing this*. Incoming message IDs are still runtime values.
- **Value-based OS errors.** Factories and supported fallible operations normally use
  `std::expected<T, E>` with `std::error_code`. Stable window/control factories use
  `CreationResult<T>`, which combines that explicit error with the permanent owner.
  A small structured error also preserves native result data when required.
  The error-contract audit is incomplete; allocation and user callbacks can still throw.
- **Explicit native resources.** WIL supplies ownership primitives. Window, Control,
  Menu, NotifyIcon and Drop manage native lifetimes; raw handles remain borrowed views.
- **One header-only library dependency** (WIL). No separate Winwrap runtime DLL;
  application dependencies and CRT linkage still determine deployment.
- **Unicode only**, UTF-16 at the boundary, `…W` APIs throughout.

## Quick start

For a created window owner, ordinary operations use pointer syntax:

```cpp
window->show();
window->set_text(L"Ready");
window->enable(true);
```

The [API conventions](CODE_CONVENTIONS.md#4-wrapper-first-apis--abstract-the-operation-preserve-the-escape-hatch)
define the boundary between wrapper operations and native interoperability.

A window with a button, wired to a click handler:

```cpp
#include <winwrap/user/desktop/window/controls/button.hpp>
#include <winwrap/user/desktop/message_loop.hpp>
#include <winwrap/user/desktop/window/window.hpp>

class MainWindow : public winwrap::user::Window<MainWindow> {
public:
    static constexpr const wchar_t* class_name = L"winwrap_demo";

    void on_created() {
        greet_ = winwrap::user::Button::create(
            {.parent = hwnd(), .id = 1, .text = L"Greet", .x = 12, .y = 12},
            [this] { set_text(L"Hello from winwrap"); });
        if (!greet_)
            winwrap::user::message_loop::quit(greet_.error().value());
    }

    void on_destroy() { winwrap::user::message_loop::quit(); }

private:
    winwrap::user::CreationResult<winwrap::user::Button> greet_;
};

int wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    auto window = MainWindow::create({.title = L"winwrap demo"});
    if (!window)
        return window.error().value();

    window->show();
    return winwrap::user::message_loop::run();
}
```

Every hook is optional — define `on_paint`, `on_size(w, h)`, `on_key_down(vk)`,
`on_mouse_move(x, y)`, `on_command(id)`, `on_focus(gained)`, `on_close` and the rest
only when you want them. Extra behaviour composes as a mixin:

```cpp
class DropTarget : public winwrap::user::Window<DropTarget, winwrap::user::FileDroppable> {
public:
    static constexpr const wchar_t* class_name = L"winwrap_drop";

    void on_files_dropped(const std::vector<std::wstring>& paths) { /* … */ }
};
```

`FileDroppable` calls `DragAcceptFiles` itself — composing the mixin is the whole
declaration, there's no matching flag to remember.

## Tray icon

The tray icon rides the same window bridge: its events arrive as an ordinary window
message you pick an id for.

```cpp
class TrayWindow : public winwrap::user::Window<TrayWindow> {
public:
    static constexpr const wchar_t* class_name = L"winwrap_tray";

    void on_created() {
        auto image = winwrap::user::icon::load(winwrap::user::SystemIcon::application, winwrap::user::IconSize::small);
        if (!image)
            return;
        auto icon = winwrap::user::NotifyIcon::create(
            {.owner = hwnd(),
             .callback_msg = tray_callback,
             .id = 1,
             .icon = image->release(),
             .tooltip = L"winwrap"});
        if (icon)
            tray_ = std::move(*icon);
    }

    LRESULT route_message(UINT msg, WPARAM wparam, LPARAM lparam) {
        if (msg == tray_callback && LOWORD(lparam) == WM_CONTEXTMENU) {
            show_tray_menu();
            return 0;
        }
        return Window::route_message(msg, wparam, lparam);
    }

private:
    static constexpr UINT tray_callback{WM_APP + 1};

    void show_tray_menu() {
        auto menu = winwrap::user::Menu::create();
        if (!menu)
            return;
        std::ignore = menu->add_item(L"Exit", [] { winwrap::user::message_loop::quit(); });
        menu->show(hwnd());
    }

    std::optional<winwrap::user::NotifyIcon> tray_;
};
```

The icon is adopted, so pass one that's safe to `DestroyIcon`: `icon::load` always
returns a private copy, never a shared system handle. Swap it later by passing another
checked `icon::load` result to `set_icon`.

## What's in the box

| Header | Gives you |
|---|---|
| `winwrap/user/desktop/window/window.hpp` | `Window<T, Mixins…>` — registration, the callback→object bridge, dispatch, teardown |
| `winwrap/user/desktop/window/control.hpp`, `winwrap/user/desktop/window/controls/*.hpp` | `Control<T, Mixins…>` and concrete `Button`, `Edit`, `Checkbox`, `ComboBox` |
| `winwrap/user/desktop/notify_icon.hpp` | `NotifyIcon` — a system-tray icon, plus the Explorer-restart re-add path (`taskbar_created_message()` → `add()`) |
| `winwrap/user/desktop/menu.hpp` | `Menu` — popup menus, items by id or by lambda |
| `winwrap/user/desktop/drop.hpp` | `Drop` — the `WM_DROPFILES` query protocol as a type |
| `winwrap/user/desktop/window/message/*.hpp` | direct window/control message routing and behaviors (`MessageRouter`, `FileDroppable`, `Paintable`, …) |
| `winwrap/user/desktop/window/notification/command/*.hpp` | control notification mixins (`notification::Click`, `TextChange`, `SelectionChange`) and parent-to-child reflection |
| `winwrap/user/desktop/window/native_window.hpp` | `window::create(NativeWindowConfig)` — any registered window class as an owned `wil::unique_hwnd` |
| `winwrap/user/desktop/message.hpp` | `message::send` / `message::post` — messages to any window (`BaseWindow::send` / `post` delegate) |
| `winwrap/user/desktop/message_loop.hpp` | `message_loop::run()` and `message_loop::quit()` |
| `winwrap/user/desktop/icon.hpp` | `icon::load` — system, module-resource and `.ico` icons as owned `wil::unique_hicon` |
| `winwrap/user/desktop/shell/change_notification.hpp` | `shell::notify_file_created` / `_deleted` / `_renamed` / `_changed`, the folder equivalents and `notify_associations_changed` — tell Explorer what changed |
| `winwrap/user/filesystem/attributes.hpp` | `filesystem::attributes` and add/remove/set — file-attribute queries and updates |
| `winwrap/user/module.hpp` | `module::current` / `loaded` / `path` and the borrowed `Module` view |
| `winwrap/shared/device/interface.hpp` | `device::Interface` — shared client/driver interface-class value |
| `winwrap/shared/device/control_code.hpp` | `device::ControlCode` — shared client/driver IOCTL value |
| `winwrap/user/device.hpp` | `Device` — user-mode present-interface paths, synchronous open and control |
| `winwrap/kernel/debug_print.hpp` | `kernel::debug_print` — DbgPrintEx diagnostics with component and level |
| `winwrap/kernel/driver/*.hpp` | `driver::Driver`, `Device`, `Queue`, and `Request` — kernel-safe borrowed KMDF adapters |
| `winwrap/user/error.hpp` | `error::last()`, `error::win32(code)`, `error::nonzero_or_last(result)`, `error::result_or_last(call)` — Win32 failures as `std::error_code` |

Types (`Window`, `Module`, `NotifyIcon`, …) live in `winwrap::user`; free-function families
live in a namespace named for their use (`winwrap::user::icon`, `winwrap::user::message`, …).

## Execution modes

Public headers follow the execution mode: `winwrap/user/` uses C++23 and WIL,
`winwrap/kernel/` uses C++20 and WDK headers, and `winwrap/shared/` contains
values available to both. Use `winwrap::user`, `winwrap::kernel`, and
`winwrap::shared` respectively. The CMake targets `winwrap::user` and
`winwrap::kernel` express the same dependency boundary; the latter is header only
and carries no user-mode import libraries. The older `winwrap::winwrap` target
remains available for user-mode consumers.

Kernel diagnostics use `winwrap/kernel/debug_print.hpp`:

```cpp
winwrap::kernel::debug_print(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
                             "Device entered D0\n");
```

The wrapper forwards the component, level, format string, and arguments to
`DbgPrintEx`. Its format and IRQL restrictions still apply; the wrapper does not
queue or defer the call.

## Device I/O

Device support has two sides. `winwrap::user::Device` owns a user-mode file handle. The
`winwrap::kernel::driver` types borrow KMDF framework handles because KMDF owns their lifetime. Shared
`winwrap::shared::device::Interface` and `winwrap::shared::device::ControlCode` values keep the published GUID and complete IOCTL
consistent across both binaries.

`Device::paths(interface_id)` snapshots all currently present interface paths;
an empty vector means none are present. `Device::open({.path = ..., .access = ...,
.share_mode = ...})` owns the `CreateFileW` handle. `control(code, input, output)`
is synchronous and reports the number of output bytes actually written; callers
must interpret and validate those bytes for their own protocol. A failed request
returns `Device::ControlError`, which preserves the native error and any partial
byte count in two forms: `bytes_returned` is bounded by the output buffer, while
`native_bytes_returned` is the exact diagnostic count from Windows and is absent only
when WinWrap rejects the request before making the native call. Input and output pointers
are preserved independently from their lengths, including storage-backed zero-length
spans. `handle()` borrows the native handle without transferring ownership. There is no
overlapped-I/O API; asynchronous requests need separate buffer and cancellation lifetimes.

Configuration Manager returns `CONFIGRET`, not a `GetLastError` code. WinWrap
uses `CM_MapCrToWin32Err` and `std::system_category()` for those failures.
Several distinct `CONFIGRET` values may map to one Win32 code; an unmapped value
becomes `ERROR_GEN_FAILURE`, so the original configuration code is lost.
If the interface list changes through three size/list attempts, `paths` returns
`ERROR_RETRY`. A malformed list returns `ERROR_INVALID_DATA`.
## Requirements

User mode requires Windows and a sufficiently recent MSVC C++23 toolchain: both `std::expected` and
explicit object parameters are required. The assessment verified MSVC 19.50 x64.
clang-cl is an intended target, but a supported-version CI matrix has not yet been
established. MinGW is not supported.

The `winwrap/kernel/driver/` headers require the WDK and KMDF. They avoid the user-mode
static library and WIL, use `NTSTATUS`, and are C++20-compatible for restricted kernel builds.

## Build

From an *x64 Native Tools* prompt:

```
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

## Use it from another project

```cmake
include(FetchContent)
FetchContent_Declare(winwrap
    GIT_REPOSITORY <repo-url>
    GIT_TAG <tag>)
FetchContent_MakeAvailable(winwrap)

target_link_libraries(your_app PRIVATE winwrap::user)
```

For source consumption, WIL comes in transitively. A full CMake install currently
also installs WIL into the prefix; explicit installed-dependency metadata and
package-manager consumption remain tracked release work.

## Design notes

[VISION.md](VISION.md) covers the design pillars and the explicit non-goals (no
layout engine, no custom-drawn widgets, no cross-platform layer, no WinRT).
[MIXINS.md](MIXINS.md) and [MESSAGE_LOOP_DESIGN.md](MESSAGE_LOOP_DESIGN.md) document
the dispatch model; [ROADMAP.md](ROADMAP.md) is the work queue.
[PLANNING.md](PLANNING.md) separates readiness gates and open design decisions
before implementation plans are authored.

## License

MIT — see [LICENSE](LICENSE).
