#pragma once

#include "winwrap/detail/user_mode.hpp"

#include "winwrap/win.hpp"

#include <shellapi.h>
#include <wil/resource.h>

#include <expected>
#include <string>
#include <system_error>

namespace winwrap {

/// Settings passed to NotifyIcon::create -- the Shell_NotifyIcon parameters, set with
/// designated initializers; omitted fields take the defaults.
struct NotifyIconConfig {
    HWND owner{};         ///< Window tray events post to (its route_message).
    UINT callback_msg{};  ///< App message id the events arrive as; use `WM_APP + n`.
    UINT id{};            ///< Identifies this icon within the owner.
    HICON icon{};         ///< Adopted -- must be safe to DestroyIcon (not a shared system icon).
    const wchar_t* tooltip{L""};  ///< Hover text; truncated past 127 chars.
};

/// A system-tray (notification-area) icon. Owns its HICON and its shell
/// registration, and attaches to a Window<T> you own: pass that window's HWND
/// plus an app-private callback message id (use `WM_APP + n`).
///
/// Tray events arrive at the owner window as that callback message. Targeting
/// `NOTIFYICON_VERSION_4`, decode them in the owner's route_message: the event is
/// `LOWORD(lParam)` (e.g. `WM_CONTEXTMENU` on right-click, `NIN_SELECT` on left-click)
/// and the icon id is `HIWORD(lParam)`. On destruction the icon is removed.
class NotifyIcon final {
public:
    /// Adds the icon to the notification area, or the Win32 error that stopped it.
    [[nodiscard]] static std::expected<NotifyIcon, std::error_code> create(
        const NotifyIconConfig& cfg);

    NotifyIcon(NotifyIcon&& other) noexcept;
    NotifyIcon& operator=(NotifyIcon&& other) noexcept;
    NotifyIcon(const NotifyIcon&) = delete;
    NotifyIcon& operator=(const NotifyIcon&) = delete;
    ~NotifyIcon();

    /// Updates the hover text (NIM_MODIFY).
    std::expected<void, std::error_code> set_tooltip(const wchar_t* text);

    /// Replaces the icon shown in the notification area (NIM_MODIFY with NIF_ICON).
    /// @param icon  The new icon, which this NotifyIcon owns from now on; load one with
    ///              icon::load. The previous icon is destroyed only after the shell accepts
    ///              the new one. On failure the tray keeps showing the previous icon and
    ///              `icon` is destroyed.
    /// @return Nothing, or the error; `ERROR_INVALID_WINDOW_HANDLE` for a moved-from NotifyIcon.
    std::expected<void, std::error_code> set_icon(wil::unique_hicon icon);

    /// Registers the icon with the shell (NIM_ADD + NIM_SETVERSION). create() calls
    /// this; call it again from the owner on taskbar_created_message() so the icon
    /// survives an Explorer restart.
    std::expected<void, std::error_code> add();

    /// The "TaskbarCreated" broadcast id Explorer sends after it restarts. Compare
    /// against it in the owner's route_message, then call add().
    [[nodiscard]] static UINT taskbar_created_message();

private:
    NotifyIcon(HWND owner, UINT callback_msg, UINT id, wil::unique_hicon icon,
               const wchar_t* tooltip);

    [[nodiscard]] NOTIFYICONDATAW make_data() const noexcept;
    void remove() noexcept;

    HWND hwnd_;
    UINT callback_msg_;
    UINT id_;
    wil::unique_hicon icon_;
    std::wstring tooltip_;
};

}  // namespace winwrap
