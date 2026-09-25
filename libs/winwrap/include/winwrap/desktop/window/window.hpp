#pragma once

#include "winwrap/win.hpp"

#include <chrono>
#include <expected>
#include <memory>
#include <system_error>
#include <utility>

#include "winwrap/desktop/window/base_window.hpp"
#include "winwrap/desktop/window/creation_result.hpp"
#include "winwrap/desktop/window/message/focus_aware.hpp"
#include "winwrap/desktop/window/message/keyboard_input.hpp"
#include "winwrap/desktop/window/message/lifecycle.hpp"
#include "winwrap/desktop/window/message/message_router.hpp"
#include "winwrap/desktop/window/message/mouse_input.hpp"
#include "winwrap/desktop/window/message/paintable.hpp"
#include "winwrap/desktop/window/message/size_change.hpp"
#include "winwrap/desktop/window/message/timer_tick.hpp"
#include "winwrap/desktop/window/message/window_command.hpp"
#include "winwrap/desktop/window/native_window.hpp"
#include "winwrap/desktop/window/notification/command/reflection.hpp"
#include "winwrap/error.hpp"
#include "winwrap/module.hpp"

namespace winwrap {

/// Per-window settings passed to Window::create -- the CreateWindowExW arguments;
/// omitted fields take the defaults below.
struct WindowConfig {
    const wchar_t* title{L""};         ///< Window title-bar text.
    DWORD style{WS_OVERLAPPEDWINDOW};  ///< Window styles; the default is not visible -- OR in
                                       ///< WS_VISIBLE, or call show().
    DWORD ex_style{0};                 ///< Extended (WS_EX_*) styles.
    int x{CW_USEDEFAULT};       ///< Left edge in pixels; CW_USEDEFAULT lets Windows place it.
    int y{CW_USEDEFAULT};       ///< Top edge in pixels; CW_USEDEFAULT lets Windows place it.
    int width{CW_USEDEFAULT};   ///< Width in pixels; CW_USEDEFAULT lets Windows size it.
    int height{CW_USEDEFAULT};  ///< Height in pixels; CW_USEDEFAULT lets Windows size it.
    HWND parent{nullptr};       ///< Owner/parent window; null for a top-level window.
};

/// CRTP base for a top-level window. Derive as
/// `class MyWindow : public winwrap::Window<MyWindow>`: the base owns class
/// registration, the WndProc->object bridge, message routing, and teardown, while
/// T defines the on_* hooks it cares about (and optionally `configure_class` /
/// `on_created`). Dispatch resolves at compile time (C++23 deducing this) -- no
/// virtual, no vtable.
///
/// Create the final object with T::create(). The returned CreationResult owns it at
/// a permanent address because Windows stores that address in the native window;
/// Window deliberately cannot be copied or moved.
///
/// Messages route to the matching hook the window defines, or to DefWindowProcW
/// when none claims it (see route_message, inherited from MessageRouter). Define
/// only the hooks you need, as **public** members; they come from the composable
/// message behaviors in <winwrap/desktop/window/message/>:
///
///   - `on_create()` / `on_close()` / `on_destroy()`  -- lifecycle
///   - `on_size(w, h)`        -- `WM_SIZE` (client width/height)
///   - `on_command(id)`       -- `WM_COMMAND` from menus and accelerators
///   - `on_paint()`           -- `WM_PAINT`
///   - `on_mouse_move(x, y)` / `on_lbutton_down(x, y)` / `on_lbutton_up(x, y)`
///   - `on_key_down(vk)`      -- `WM_KEYDOWN` (virtual-key code)
///   - `on_focus(gained)`     -- `WM_SETFOCUS` (true) / `WM_KILLFOCUS` (false)
///   - `on_timer(id)`         -- `WM_TIMER` from start_timer
///
/// For a message with a runtime id (e.g. a tray callback), shadow route_message
/// in T and delegate the rest with `Window::route_message`. A custom router may
/// call default_proc directly when it deliberately declines the current message.
///
/// Extra mixins compose *after* the built-ins (first-match-wins, so a built-in
/// hook always beats an extra on an overlapping message); to intercept a message
/// a built-in claims, shadow route_message instead.
///
/// @tparam T       The derived window type. Must provide
///                 `static constexpr const wchar_t* class_name` and be
///                 default-constructible.
/// @tparam Mixins  Extra window mixins to compose (e.g. FileDroppable), tried
///                 after the built-ins in the order given.
template <typename T, typename... Mixins>
class Window
    : public BaseWindow,
      public MessageRouter<Lifecycle, SizeChange, WindowCommand, notification::CommandReflection,
                           Paintable, MouseInput, KeyboardInput, FocusAware, TimerTick, Mixins...> {
public:
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    /// Constructs the final object, creates its window, and runs the post-create hook.
    /// @param cfg  Per-window settings (title, style, geometry, parent).
    /// @return     Stable owner of the live window, or the Win32 creation error.
    [[nodiscard]] static CreationResult<T> create(const WindowConfig& cfg = {}) {
        auto self = std::unique_ptr<T>{new T{}};
        if (auto made = self->create_window(cfg); !made)
            return CreationResult<T>{made.error()};
        self->on_created();
        return CreationResult<T>{std::move(self)};
    }

    /// Delegates one message to this window's native fallback (DefWindowProcW).
    /// Use from an advanced route_message override when it deliberately declines
    /// the current message; ordinary routing calls it automatically.
    [[nodiscard]] LRESULT default_proc(UINT msg, WPARAM wparam, LPARAM lparam) {
        return DefWindowProcW(hwnd(), msg, wparam, lparam);
    }

    /// Starts timer `id` on this window, or restarts it with a new interval (SetTimer).
    /// Each tick arrives as `WM_TIMER` and reaches `on_timer(id)`. Timers stop when the
    /// window is destroyed; stop one earlier with stop_timer.
    /// @param interval  Time between ticks. Windows raises values below
    ///                  `USER_TIMER_MINIMUM` (10 ms) to that minimum.
    /// @return Nothing, or the Win32 error; `ERROR_INVALID_PARAMETER` when `interval` is
    ///         negative or above `USER_TIMER_MAXIMUM`.
    std::expected<void, std::error_code> start_timer(UINT_PTR id,
                                                     std::chrono::milliseconds interval) {
        if (interval.count() < 0 || interval.count() > USER_TIMER_MAXIMUM)
            return std::unexpected(error::win32(ERROR_INVALID_PARAMETER));
        // SetTimer(nullptr, ...) would start a thread timer instead of failing.
        if (!hwnd())
            return std::unexpected(error::win32(ERROR_INVALID_WINDOW_HANDLE));
        if (SetTimer(hwnd(), id, static_cast<UINT>(interval.count()), nullptr) == 0)
            return std::unexpected(error::last());
        return {};
    }

    /// Stops timer `id` (KillTimer). A tick already waiting in the message queue is
    /// still delivered.
    /// @return Nothing, or the Win32 error, e.g. when no such timer is running.
    std::expected<void, std::error_code> stop_timer(UINT_PTR id) {
        // KillTimer(nullptr, id) would stop an unrelated thread timer with the same id.
        if (!hwnd())
            return std::unexpected(error::win32(ERROR_INVALID_WINDOW_HANDLE));
        return error::nonzero_or_last(KillTimer(hwnd(), id));
    }

protected:
    Window() = default;
    ~Window() {
        if (hwnd()) {
            // Detach first so the destroy messages don't dispatch into a
            // half-destroyed object.
            SetWindowLongPtrW(hwnd(), GWLP_USERDATA, 0);
            DestroyWindow(hwnd());
        }
    }

    /// Hook to customise the window class before registration (icon, background
    /// brush, class styles). Shadow in T; the base default changes nothing.
    void configure_class(WNDCLASSW& /*wc*/) {}

    /// Hook called once, right after the window exists. Shadow in T for setup
    /// that needs a live HWND.
    void on_created() {}

private:
    std::expected<void, std::error_code> create_window(const WindowConfig& cfg) {
        // --- Registration (per class name): sensible defaults, then let T tweak.
        WNDCLASSW wc{};
        wc.lpfnWndProc = window_proc;
        wc.hInstance = instance_;
        wc.lpszClassName = T::class_name;
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        static_cast<T*>(this)->configure_class(wc);

        if (RegisterClassW(&wc) == 0) {
            // A class already registered under this name is fine -- anything else
            // is a real failure.
            if (auto ec = error::last(); ec.value() != ERROR_CLASS_ALREADY_EXISTS)
                return std::unexpected(ec);
        }

        // --- Creation (per window): `this` rides through so the static callback
        // can recover the object in WM_NCCREATE.
        // The destructor, not the handle, destroys the window: it must detach first.
        return window::create({.class_name = T::class_name,
                               .title = cfg.title,
                               .style = cfg.style,
                               .ex_style = cfg.ex_style,
                               .x = cfg.x,
                               .y = cfg.y,
                               .width = cfg.width,
                               .height = cfg.height,
                               .parent = cfg.parent,
                               .create_param = static_cast<T*>(this)})
            .transform([](wil::unique_hwnd window) { window.release(); });
    }

    static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        T* self{};
        if (msg == WM_NCCREATE) {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lparam);
            self = static_cast<T*>(cs->lpCreateParams);
            self->attach(hwnd);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        } else {
            self = reinterpret_cast<T*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (self) {
            LRESULT result = self->route_message(msg, wparam, lparam);
            if (msg == WM_NCDESTROY) {
                // The window is gone; sever the link so the destructor won't
                // DestroyWindow a dead handle.
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
                self->detach();
            }
            return result;
        }
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    const HINSTANCE instance_{module::current()};
};

}  // namespace winwrap
