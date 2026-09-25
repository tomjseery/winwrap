#pragma once

#include "winwrap/win.hpp"

#include <expected>
#include <string>
#include <system_error>

#include "winwrap/desktop/message.hpp"
#include "winwrap/error.hpp"

namespace winwrap {

/// Non-owning base for everything backed by an HWND. The operations common to any
/// window -- its caption, enabled/visible state, and so on -- live here once, so
/// Window<T> and Control<T> share a single implementation instead of each re-exposing
/// their own subset. Mirrors ATL's CWindow: it stores the handle and operates on it,
/// but never manages its lifetime -- the derived wrapper owns that (Window destroys the
/// window; Control merely unsubclasses it). Holds no vtable; destruction always runs
/// through the derived unique_ptr, so the non-virtual destructor is correct.
class BaseWindow {
public:
    /// The underlying window handle, or nullptr once the window is destroyed.
    [[nodiscard]] HWND hwnd() const noexcept { return hwnd_; }

    /// This window's caption (title-bar text, button label, static text, edit contents)
    /// as UTF-16.
    [[nodiscard]] std::wstring text() const {
        const int len = GetWindowTextLengthW(hwnd_);
        std::wstring buf(static_cast<size_t>(len), L'\0');
        if (len > 0)
            GetWindowTextW(hwnd_, buf.data(), len + 1);  // writes len chars + the trailing NUL
        return buf;
    }

    /// Sets this window's caption (SetWindowTextW).
    void set_text(const wchar_t* text) noexcept { SetWindowTextW(hwnd_, text); }

    /// Enables or disables this window (EnableWindow).
    void enable(bool enabled) noexcept { EnableWindow(hwnd_, enabled); }

    /// Shows the window (or applies another ShowWindow command).
    /// @param cmd  A ShowWindow command; defaults to SW_SHOW. Pass wWinMain's
    ///             nShowCmd on first display to honour how the app was launched.
    void show(int cmd = SW_SHOW) noexcept { ShowWindow(hwnd_, cmd); }

    /// Whether the window is currently visible (WS_VISIBLE set up its parent chain).
    [[nodiscard]] bool is_visible() const noexcept { return IsWindowVisible(hwnd_) != FALSE; }

    /// Hides the window (`ShowWindow(SW_HIDE)`); show() makes it visible again.
    void hide() noexcept { ShowWindow(hwnd_, SW_HIDE); }

    /// Whether the window accepts mouse and keyboard input (IsWindowEnabled).
    [[nodiscard]] bool is_enabled() const noexcept { return IsWindowEnabled(hwnd_) != FALSE; }

    /// The client area in client coordinates (GetClientRect): `left` and `top` are always
    /// 0, so `right` and `bottom` are its width and height.
    [[nodiscard]] std::expected<RECT, std::error_code> client_rect() const {
        RECT rect{};
        return error::nonzero_or_last(GetClientRect(hwnd_, &rect)).transform([&] { return rect; });
    }

    /// The whole window, including its frame, in screen coordinates (GetWindowRect).
    [[nodiscard]] std::expected<RECT, std::error_code> window_rect() const {
        RECT rect{};
        return error::nonzero_or_last(GetWindowRect(hwnd_, &rect)).transform([&] { return rect; });
    }

    /// Moves the window's top-left corner without resizing, reordering or activating it
    /// (`SetWindowPos` with `SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE`).
    /// @note A top-level window moves in screen coordinates; a child window moves in its
    ///       parent's client coordinates.
    std::expected<void, std::error_code> move(int x, int y) {
        return error::nonzero_or_last(
            SetWindowPos(hwnd_, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE));
    }

    /// Resizes the whole window, frame included, without moving, reordering or activating
    /// it (`SetWindowPos` with `SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE`).
    std::expected<void, std::error_code> resize(int width, int height) {
        return error::nonzero_or_last(SetWindowPos(hwnd_, nullptr, 0, 0, width, height,
                                  SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE));
    }

    /// Marks the whole client area for repainting (`InvalidateRect(hwnd, nullptr, erase)`).
    /// `WM_PAINT` follows once the thread's message queue is otherwise empty.
    /// @param erase  Whether the background is erased (`WM_ERASEBKGND`) before painting.
    void invalidate(bool erase = true) noexcept {
        // A null HWND would invalidate every window on the desktop.
        if (hwnd_)
            InvalidateRect(hwnd_, nullptr, erase);
    }

    /// Gives this window the keyboard focus (SetFocus).
    /// @return Nothing, or the Win32 error, e.g. when the window belongs to another thread.
    ///         SetFocus's null result also means "nothing had focus before", so only a
    ///         null result with a recorded error counts as failure.
    std::expected<void, std::error_code> focus() {
        // SetFocus(nullptr) would clear the thread's focus instead of failing.
        if (!hwnd_)
            return std::unexpected(error::win32(ERROR_INVALID_WINDOW_HANDLE));
        return error::result_or_last([&] { return SetFocus(hwnd_); }).transform([](HWND) {});
    }

    /// Whether this window has the calling thread's keyboard focus (GetFocus).
    [[nodiscard]] bool has_focus() const noexcept { return hwnd_ && GetFocus() == hwnd_; }

    /// Asks the window to close as if the user clicked its close button: posts `WM_CLOSE`,
    /// so `on_close` (or the default destroy) runs later from the message loop, never
    /// inside this call.
    std::expected<void, std::error_code> request_close() { return post(WM_CLOSE); }

    /// Sets the font this window draws its text with (`WM_SETFONT`).
    /// Native controls and dialogs store it; a plain window's `DefWindowProcW` ignores it,
    /// so a Window<T> that paints text chooses its font in `on_paint` instead.
    /// @param font    The font; it is borrowed, not owned, so it must outlive its use by
    ///                this window. A stock font (`GetStockObject`) never needs freeing.
    /// @param redraw  Whether the window repaints immediately.
    void set_font(HFONT font, bool redraw = true) const {
        send(WM_SETFONT, reinterpret_cast<WPARAM>(font), redraw ? TRUE : FALSE);
    }

    /// The font this window draws its text with (`WM_GETFONT`), or null for the system font
    /// (always null for a plain window, which stores no font).
    [[nodiscard]] HFONT font() const { return reinterpret_cast<HFONT>(send(WM_GETFONT)); }

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
        return window_long(GWL_STYLE);
    }

    /// The window's `WS_EX_*` extended style bits (`GetWindowLongPtrW(GWL_EXSTYLE)`).
    [[nodiscard]] std::expected<DWORD, std::error_code> ex_style() const {
        return window_long(GWL_EXSTYLE);
    }

protected:
    BaseWindow() = default;
    BaseWindow(const BaseWindow&) = default;
    BaseWindow(BaseWindow&&) = default;
    BaseWindow& operator=(const BaseWindow&) = default;
    BaseWindow& operator=(BaseWindow&&) = default;
    ~BaseWindow() = default;  // non-owning: never touches the handle

    /// Binds the handle once the OS has created the window (called from the derived
    /// WndProc / subclass bridge).
    void attach(HWND h) noexcept { hwnd_ = h; }

    /// Severs the handle when the window is gone, so no stale operation can fire on a
    /// dead HWND.
    void detach() noexcept { hwnd_ = nullptr; }

private:
    // A zero window long is also a legitimate value (error::result_or_last).
    [[nodiscard]] std::expected<DWORD, std::error_code> window_long(int index) const {
        return error::result_or_last([&] { return GetWindowLongPtrW(hwnd_, index); })
            .transform([](LONG_PTR value) { return static_cast<DWORD>(value); });
    }

    HWND hwnd_{};
};

}  // namespace winwrap
