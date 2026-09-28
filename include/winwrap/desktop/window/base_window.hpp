#pragma once

#include "winwrap/detail/user_mode.hpp"

#include "winwrap/win.hpp"

#include <commctrl.h>
#include <shellapi.h>
#include <wil/resource.h>

#include <chrono>
#include <expected>
#include <string>
#include <system_error>

#include "winwrap/desktop/message.hpp"
#include "winwrap/error.hpp"
#include "winwrap/module.hpp"

/// Operations on any window by its raw HWND, whichever library created it. Each owns its
/// native call; the BaseWindow, Window and Control members delegate to these, so code
/// holding only an HWND gets the same guards and error reporting.
namespace winwrap::window {

/// Destroys `hwnd` and its child windows (DestroyWindow), sending `WM_DESTROY` and
/// `WM_NCDESTROY` before it returns. Only the thread that created the window may destroy it.
/// @return Nothing, or the Win32 error, e.g. `ERROR_INVALID_WINDOW_HANDLE` for a window that
///         no longer exists.
inline std::expected<void, std::error_code> destroy(HWND hwnd) {
    return error::nonzero_or_last(DestroyWindow(hwnd));
}

/// The default processing for a message no handler claims (DefWindowProcW). A window
/// procedure returns this for every message it does not handle itself.
inline LRESULT default_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

/// Installs `procedure` as subclass `id` of `hwnd`, or replaces the reference data of an
/// existing one with the same procedure and id (SetWindowSubclass). Each subclass is
/// identified by its procedure and id, so several chain on one window without disturbing
/// `GWLP_WNDPROC` or `GWLP_USERDATA`; the procedure receives `data` on every call. Only the
/// thread that owns the window may subclass it.
/// @return Nothing, or the Win32 error; `ERROR_GEN_FAILURE` when the call fails without
///         recording one, which SetWindowSubclass does not promise to.
[[nodiscard]] inline std::expected<void, std::error_code> subclass(HWND hwnd,
                                                                   SUBCLASSPROC procedure,
                                                                   UINT_PTR id, DWORD_PTR data) {
    return error::result_or_last([&] { return SetWindowSubclass(hwnd, procedure, id, data); })
        .and_then([](BOOL installed) -> std::expected<void, std::error_code> {
            if (!installed)
                return std::unexpected(error::win32(ERROR_GEN_FAILURE));
            return {};
        });
}

/// Removes subclass `id` installed with `procedure` (RemoveWindowSubclass).
/// @return Whether a matching subclass was installed and is now removed. Windows records no
///         error code for a failed removal.
inline bool remove_subclass(HWND hwnd, SUBCLASSPROC procedure, UINT_PTR id) noexcept {
    return RemoveWindowSubclass(hwnd, procedure, id) != FALSE;
}

/// Passes a message on to the next subclass in the chain, or finally to the window's own
/// procedure (DefSubclassProc). A subclass procedure returns this for every message it does
/// not handle itself -- the subclass counterpart of default_proc.
inline LRESULT default_subclass_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    return DefSubclassProc(hwnd, msg, wparam, lparam);
}

/// The window value at `index` (GetWindowLongPtrW): a `GWL_*`/`GWLP_*` index such as
/// `GWL_STYLE` or `GWLP_USERDATA`, or a byte offset into the class's extra window memory.
/// @return The value, possibly zero, or the Win32 error when the read failed.
[[nodiscard]] inline std::expected<LONG_PTR, std::error_code> long_ptr(HWND hwnd, int index) {
    return error::result_or_last([&] { return GetWindowLongPtrW(hwnd, index); });
}

/// Replaces the window value at `index` (SetWindowLongPtrW; see long_ptr for the indexes).
/// @return The previous value, possibly zero, or the Win32 error when the write failed.
inline std::expected<LONG_PTR, std::error_code> set_long_ptr(HWND hwnd, int index, LONG_PTR value) {
    return error::result_or_last([&] { return SetWindowLongPtrW(hwnd, index, value); });
}

/// The window's `WS_*` style bits (`GetWindowLongPtrW(GWL_STYLE)`).
[[nodiscard]] inline std::expected<DWORD, std::error_code> style(HWND hwnd) {
    return long_ptr(hwnd, GWL_STYLE).transform([](LONG_PTR value) {
        return static_cast<DWORD>(value);
    });
}

/// The window's `WS_EX_*` extended style bits (`GetWindowLongPtrW(GWL_EXSTYLE)`).
[[nodiscard]] inline std::expected<DWORD, std::error_code> ex_style(HWND hwnd) {
    return long_ptr(hwnd, GWL_EXSTYLE).transform([](LONG_PTR value) {
        return static_cast<DWORD>(value);
    });
}

/// The window's caption (title-bar text, button label, static text, edit contents) as UTF-16.
[[nodiscard]] inline std::wstring text(HWND hwnd) {
    const int len = GetWindowTextLengthW(hwnd);
    if (len <= 0)
        return {};
    std::wstring buf(static_cast<size_t>(len), L'\0');
    const int copied = GetWindowTextW(hwnd, buf.data(), static_cast<int>(buf.size() + 1));
    buf.resize(static_cast<size_t>(copied > 0 ? copied : 0));
    return buf;
}

/// Sets the window's caption (SetWindowTextW).
inline void set_text(HWND hwnd, const wchar_t* text) noexcept {
    SetWindowTextW(hwnd, text);
}

/// Enables or disables the window (EnableWindow).
inline void enable(HWND hwnd, bool enabled) noexcept {
    EnableWindow(hwnd, enabled);
}

/// Whether the window accepts mouse and keyboard input (IsWindowEnabled).
[[nodiscard]] inline bool is_enabled(HWND hwnd) noexcept {
    return IsWindowEnabled(hwnd) != FALSE;
}

/// Shows the window, or applies another ShowWindow command.
/// @param cmd  A ShowWindow command; defaults to SW_SHOW. Pass wWinMain's nShowCmd on
///             first display to honour how the app was launched.
inline void show(HWND hwnd, int cmd = SW_SHOW) noexcept {
    ShowWindow(hwnd, cmd);
}

/// Hides the window (`ShowWindow(SW_HIDE)`); show() makes it visible again.
inline void hide(HWND hwnd) noexcept {
    show(hwnd, SW_HIDE);
}

/// Whether the window is currently visible (WS_VISIBLE set up its parent chain).
[[nodiscard]] inline bool is_visible(HWND hwnd) noexcept {
    return IsWindowVisible(hwnd) != FALSE;
}

/// The client area in client coordinates (GetClientRect): `left` and `top` are always 0,
/// so `right` and `bottom` are its width and height.
[[nodiscard]] inline std::expected<RECT, std::error_code> client_rect(HWND hwnd) {
    RECT rect{};
    return error::nonzero_or_last(GetClientRect(hwnd, &rect)).transform([&] { return rect; });
}

/// The whole window, including its frame, in screen coordinates (GetWindowRect).
[[nodiscard]] inline std::expected<RECT, std::error_code> window_rect(HWND hwnd) {
    RECT rect{};
    return error::nonzero_or_last(GetWindowRect(hwnd, &rect)).transform([&] { return rect; });
}

/// Moves the window's top-left corner without resizing, reordering or activating it
/// (`SetWindowPos` with `SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE`).
/// @note A top-level window moves in screen coordinates; a child window moves in its
///       parent's client coordinates.
inline std::expected<void, std::error_code> move(HWND hwnd, int x, int y) {
    return error::nonzero_or_last(
        SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE));
}

/// Resizes the whole window, frame included, without moving, reordering or activating it
/// (`SetWindowPos` with `SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE`).
inline std::expected<void, std::error_code> resize(HWND hwnd, int width, int height) {
    return error::nonzero_or_last(SetWindowPos(hwnd, nullptr, 0, 0, width, height,
                                               SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE));
}

/// Marks the whole client area for repainting (`InvalidateRect(hwnd, nullptr, erase)`).
/// `WM_PAINT` follows once the thread's message queue is otherwise empty. A null `hwnd`
/// does nothing.
/// @param erase  Whether the background is erased (`WM_ERASEBKGND`) before painting.
inline void invalidate(HWND hwnd, bool erase = true) noexcept {
    // A null HWND would invalidate every window on the desktop.
    if (hwnd)
        InvalidateRect(hwnd, nullptr, erase);
}

/// Gives the window the keyboard focus (SetFocus).
/// @return Nothing, or the Win32 error, e.g. when the window belongs to another thread;
///         `ERROR_INVALID_WINDOW_HANDLE` for a null `hwnd`. SetFocus's null result also means
///         "nothing had focus before", so only a null result with a recorded error counts
///         as failure.
inline std::expected<void, std::error_code> focus(HWND hwnd) {
    // SetFocus(nullptr) would clear the thread's focus instead of failing.
    if (!hwnd)
        return std::unexpected(error::win32(ERROR_INVALID_WINDOW_HANDLE));
    return error::result_or_last([&] { return SetFocus(hwnd); }).transform([](HWND) {});
}

/// Brings the window's thread to the foreground and activates the window
/// (SetForegroundWindow). Windows lets a process do this only in limited circumstances, such
/// as while handling the user's latest input, so it can be refused.
/// @return Whether the window was brought to the foreground. Windows records no error code
///         for a refusal.
inline bool set_foreground(HWND hwnd) noexcept {
    return SetForegroundWindow(hwnd) != FALSE;
}

/// Registers or unregisters the window to receive files dropped from Explorer as
/// `WM_DROPFILES` (DragAcceptFiles, which sets or clears `WS_EX_ACCEPTFILES`).
/// Window types compose the FileDroppable mixin instead, which calls this for them.
inline void accept_files(HWND hwnd, bool accept = true) noexcept {
    DragAcceptFiles(hwnd, accept ? TRUE : FALSE);
}

/// Whether the window has the calling thread's keyboard focus (GetFocus); false for null.
[[nodiscard]] inline bool has_focus(HWND hwnd) noexcept {
    return hwnd && GetFocus() == hwnd;
}

/// Asks the window to close as if the user clicked its close button: posts `WM_CLOSE`, so
/// the window's close handling runs later from its message loop, never inside this call.
inline std::expected<void, std::error_code> request_close(HWND hwnd) {
    return message::post(hwnd, WM_CLOSE);
}

/// Sets the font the window draws its text with (`WM_SETFONT`).
/// Native controls and dialogs store it; a plain window's `DefWindowProcW` ignores it.
/// @param font    The font; it is borrowed, not owned, so it must outlive its use by the
///                window. A stock font (`GetStockObject`) never needs freeing.
/// @param redraw  Whether the window repaints immediately.
inline void set_font(HWND hwnd, HFONT font, bool redraw = true) {
    message::send(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), redraw ? TRUE : FALSE);
}

/// The font the window draws its text with (`WM_GETFONT`), or null for the system font.
[[nodiscard]] inline HFONT font(HWND hwnd) {
    return reinterpret_cast<HFONT>(message::send(hwnd, WM_GETFONT));
}

/// Starts timer `id` on `hwnd`, or restarts it with a new interval (SetTimer). Each tick
/// arrives as `WM_TIMER` with `id` in `wParam`. Timers stop when the window is destroyed;
/// stop one earlier with stop_timer.
/// @param interval  Time between ticks. Windows raises values below `USER_TIMER_MINIMUM`
///                  (10 ms) to that minimum.
/// @return Nothing, or the Win32 error; `ERROR_INVALID_PARAMETER` when `interval` is
///         negative or above `USER_TIMER_MAXIMUM`, and `ERROR_INVALID_WINDOW_HANDLE` for a
///         null `hwnd`.
inline std::expected<void, std::error_code> start_timer(HWND hwnd, UINT_PTR id,
                                                        std::chrono::milliseconds interval) {
    if (interval.count() < 0 || interval.count() > USER_TIMER_MAXIMUM)
        return std::unexpected(error::win32(ERROR_INVALID_PARAMETER));
    // SetTimer(nullptr, ...) would start a thread timer instead of failing.
    if (!hwnd)
        return std::unexpected(error::win32(ERROR_INVALID_WINDOW_HANDLE));
    if (SetTimer(hwnd, id, static_cast<UINT>(interval.count()), nullptr) == 0)
        return std::unexpected(error::last());
    return {};
}

/// Stops timer `id` on `hwnd` (KillTimer). A tick already waiting in the message queue is
/// still delivered.
/// @return Nothing, or the Win32 error, e.g. when no such timer is running;
///         `ERROR_INVALID_WINDOW_HANDLE` for a null `hwnd`.
inline std::expected<void, std::error_code> stop_timer(HWND hwnd, UINT_PTR id) {
    // KillTimer(nullptr, id) would stop an unrelated thread timer with the same id.
    if (!hwnd)
        return std::unexpected(error::win32(ERROR_INVALID_WINDOW_HANDLE));
    return error::nonzero_or_last(KillTimer(hwnd, id));
}

}  // namespace winwrap::window

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

/// Non-owning base for everything backed by an HWND. The operations common to any
/// window -- its caption, enabled/visible state, and so on -- live here once, so
/// Window<T> and Control<T> share a single implementation instead of each re-exposing
/// their own subset. Each member delegates to the winwrap::window operation of the same
/// name. Mirrors ATL's CWindow: it stores the handle and operates on it, but never
/// manages its lifetime -- the derived wrapper owns and destroys it. Holds no vtable;
/// CreationResult destroys the final derived type, so the non-virtual destructor is correct.
class BaseWindow {
public:
    /// The underlying window handle, or nullptr once the window is destroyed.
    [[nodiscard]] HWND hwnd() const noexcept { return hwnd_; }

    /// This window's caption (title-bar text, button label, static text, edit contents)
    /// as UTF-16.
    [[nodiscard]] std::wstring text() const { return window::text(hwnd_); }

    /// Sets this window's caption (SetWindowTextW).
    void set_text(const wchar_t* text) noexcept { window::set_text(hwnd_, text); }

    /// Enables or disables this window (EnableWindow).
    void enable(bool enabled) noexcept { window::enable(hwnd_, enabled); }

    /// Shows the window (or applies another ShowWindow command).
    /// @param cmd  A ShowWindow command; defaults to SW_SHOW. Pass wWinMain's
    ///             nShowCmd on first display to honour how the app was launched.
    void show(int cmd = SW_SHOW) noexcept { window::show(hwnd_, cmd); }

    /// Whether the window is currently visible (WS_VISIBLE set up its parent chain).
    [[nodiscard]] bool is_visible() const noexcept { return window::is_visible(hwnd_); }

    /// Hides the window (`ShowWindow(SW_HIDE)`); show() makes it visible again.
    void hide() noexcept { window::hide(hwnd_); }

    /// Whether the window accepts mouse and keyboard input (IsWindowEnabled).
    [[nodiscard]] bool is_enabled() const noexcept { return window::is_enabled(hwnd_); }

    /// The client area in client coordinates (GetClientRect): `left` and `top` are always
    /// 0, so `right` and `bottom` are its width and height.
    [[nodiscard]] std::expected<RECT, std::error_code> client_rect() const {
        return window::client_rect(hwnd_);
    }

    /// The whole window, including its frame, in screen coordinates (GetWindowRect).
    [[nodiscard]] std::expected<RECT, std::error_code> window_rect() const {
        return window::window_rect(hwnd_);
    }

    /// Moves the window's top-left corner without resizing, reordering or activating it.
    /// @note A top-level window moves in screen coordinates; a child window moves in its
    ///       parent's client coordinates.
    std::expected<void, std::error_code> move(int x, int y) { return window::move(hwnd_, x, y); }

    /// Resizes the whole window, frame included, without moving, reordering or activating it.
    std::expected<void, std::error_code> resize(int width, int height) {
        return window::resize(hwnd_, width, height);
    }

    /// Marks the whole client area for repainting (InvalidateRect). `WM_PAINT` follows once
    /// the thread's message queue is otherwise empty.
    /// @param erase  Whether the background is erased (`WM_ERASEBKGND`) before painting.
    void invalidate(bool erase = true) noexcept { window::invalidate(hwnd_, erase); }

    /// Gives this window the keyboard focus (SetFocus).
    /// @return Nothing, or the Win32 error, e.g. when the window belongs to another thread.
    std::expected<void, std::error_code> focus() { return window::focus(hwnd_); }

    /// Whether this window has the calling thread's keyboard focus (GetFocus).
    [[nodiscard]] bool has_focus() const noexcept { return window::has_focus(hwnd_); }

    /// Brings this window to the foreground and activates it (SetForegroundWindow).
    /// @return Whether Windows allowed it; a process may take the foreground only in limited
    ///         circumstances, such as while handling the user's latest input.
    bool set_foreground() noexcept { return window::set_foreground(hwnd_); }

    /// Asks the window to close as if the user clicked its close button: posts `WM_CLOSE`,
    /// so `on_close` (or the default destroy) runs later from the message loop, never
    /// inside this call.
    std::expected<void, std::error_code> request_close() { return window::request_close(hwnd_); }

    /// Sets the font this window draws its text with (`WM_SETFONT`).
    /// Native controls and dialogs store it; a plain window's `DefWindowProcW` ignores it,
    /// so a Window<T> that paints text chooses its font in `on_paint` instead.
    /// @param font    The font; it is borrowed, not owned, so it must outlive its use by
    ///                this window. A stock font (`GetStockObject`) never needs freeing.
    /// @param redraw  Whether the window repaints immediately.
    void set_font(HFONT font, bool redraw = true) const { window::set_font(hwnd_, font, redraw); }

    /// The font this window draws its text with (`WM_GETFONT`), or null for the system font
    /// (always null for a plain window, which stores no font).
    [[nodiscard]] HFONT font() const { return window::font(hwnd_); }

    /// Sends `msg` to this window and waits for the result (message::send).
    LRESULT send(UINT msg, WPARAM wparam = 0, LPARAM lparam = 0) const {
        return message::send(hwnd_, msg, wparam, lparam);
    }

    /// Queues `msg` for this window and returns immediately (message::post); safe from any
    /// thread, e.g. a worker reporting `WM_APP + n` back to its window.
    [[nodiscard]] std::expected<void, std::error_code> post(UINT msg, WPARAM wparam = 0,
                                                            LPARAM lparam = 0) const {
        return message::post(hwnd_, msg, wparam, lparam);
    }

    /// The window's `WS_*` style bits (`GetWindowLongPtrW(GWL_STYLE)`).
    [[nodiscard]] std::expected<DWORD, std::error_code> style() const {
        return window::style(hwnd_);
    }

    /// The window's `WS_EX_*` extended style bits (`GetWindowLongPtrW(GWL_EXSTYLE)`).
    [[nodiscard]] std::expected<DWORD, std::error_code> ex_style() const {
        return window::ex_style(hwnd_);
    }

protected:
    BaseWindow() = default;
    BaseWindow(const BaseWindow&) = default;
    BaseWindow(BaseWindow&&) = default;
    BaseWindow& operator=(const BaseWindow&) = default;
    BaseWindow& operator=(BaseWindow&&) = default;
    ~BaseWindow() = default;  // non-owning: never touches the handle

    /// Creates a window of the registered class `class_name` in the running executable
    /// (CreateWindowExW), owned by the returned handle until the caller binds it. Native
    /// messages such as `WM_NCCREATE` and `WM_CREATE` reach the class's window procedure
    /// before this returns. The window never gets a menu bar.
    /// @param child_id      A child window's id (its `WM_COMMAND` id); 0 for a top-level window.
    /// @param create_param  Passed to `WM_NCCREATE`/`WM_CREATE` as `lpCreateParams`.
    /// @return The owned window, or the Win32 error: e.g. `ERROR_CANNOT_FIND_WND_CLASS`, or
    ///         `ERROR_INVALID_HANDLE` when a window procedure rejects creation. A creation
    ///         refused without any recorded error is `ERROR_CANCELLED`.
    [[nodiscard]] static std::expected<wil::unique_hwnd, std::error_code> create_hwnd(
        const wchar_t* class_name, const WindowConfig& config, UINT child_id, void* create_param) {
        return error::result_or_last([&] {
                   // A child window's id travels in the HMENU parameter; 0 is no menu.
                   return CreateWindowExW(config.ex_style, class_name, config.title, config.style,
                                          config.x, config.y, config.width, config.height,
                                          config.parent,
                                          reinterpret_cast<HMENU>(static_cast<UINT_PTR>(child_id)),
                                          module::current(), create_param);
               })
            .and_then([](HWND hwnd) -> std::expected<wil::unique_hwnd, std::error_code> {
                // Null with no recorded error: a window procedure refused creation silently.
                if (!hwnd)
                    return std::unexpected(error::win32(ERROR_CANCELLED));
                return wil::unique_hwnd{hwnd};
            });
    }

    /// Binds the handle once the OS has created the window (called from the derived
    /// WndProc / subclass bridge).
    void attach(HWND h) noexcept { hwnd_ = h; }

    /// Severs the handle when the window is gone, so no stale operation can fire on a
    /// dead HWND.
    void detach() noexcept { hwnd_ = nullptr; }

private:
    HWND hwnd_{};
};

}  // namespace winwrap
