#pragma once

#include "winwrap/win.hpp"

#include <expected>
#include <system_error>

#include <wil/resource.h>

namespace winwrap {

/// Settings passed to native_window::create -- the CreateWindowExW arguments by name, set
/// with designated initializers; omitted fields take the defaults below.
struct NativeWindowConfig {
    const wchar_t* class_name{};  ///< A registered class: a system class such as `L"BUTTON"`
                                  ///< or one registered in the running executable.
    const wchar_t* title{L""};    ///< Title-bar text, or a control's caption.
    DWORD style{};                ///< `WS_*` styles; include `WS_CHILD` for a child window.
    DWORD ex_style{};             ///< Extended (`WS_EX_*`) styles.
    int x{CW_USEDEFAULT};         ///< Left edge; CW_USEDEFAULT lets Windows place a top-level window.
    int y{CW_USEDEFAULT};         ///< Top edge; CW_USEDEFAULT lets Windows place a top-level window.
    int width{CW_USEDEFAULT};     ///< Width; CW_USEDEFAULT lets Windows size a top-level window.
    int height{CW_USEDEFAULT};    ///< Height; CW_USEDEFAULT lets Windows size a top-level window.
    HWND parent{};                ///< Parent (child window) or owner (top-level window).
    HMENU menu{};                 ///< A top-level window's menu bar; ignored with `WS_CHILD`.
    UINT child_id{};              ///< A child window's id (its `WM_COMMAND` id); needs `WS_CHILD`.
    void* create_param{};         ///< Passed to `WM_NCCREATE`/`WM_CREATE` as `lpCreateParams`.
};

/// Creating windows directly from any registered window class.
namespace native_window {

/// Creates a window (CreateWindowExW), owned by the returned handle, which destroys it
/// (DestroyWindow) when reset. Native messages such as `WM_NCCREATE` and `WM_CREATE` are sent
/// to the class's window procedure before this returns.
///
/// `CreateWindowExW` reuses one argument for two meanings: a top-level window's menu or a
/// child window's id. This chooses between `menu` and `child_id` from `WS_CHILD` in `style`.
/// The window belongs to the running executable (module::current()).
///
/// @return The owned window, or the Win32 error: e.g. `ERROR_CANNOT_FIND_WND_CLASS`, or
///         `ERROR_INVALID_HANDLE` when a window procedure rejects `WM_NCCREATE`/`WM_CREATE`.
///         A creation refused without any recorded error is `ERROR_CANCELLED`.
/// @note A child window is also destroyed with its parent; `release()` the handle when the
///       parent owns its lifetime.
[[nodiscard]] std::expected<wil::unique_hwnd, std::error_code> create(
    const NativeWindowConfig& config);

}  // namespace native_window

}  // namespace winwrap
