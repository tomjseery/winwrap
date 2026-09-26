#pragma once

#include "winwrap/user/mode.hpp"
#include "winwrap/user/win.hpp"

#include <wil/resource.h>

#include <expected>
#include <filesystem>
#include <system_error>

namespace winwrap::user {

/// The standard system icons (the `IDI_*` / `OIC_*` resources).
enum class SystemIcon : WORD {
    application = 32512,  ///< IDI_APPLICATION
    error = 32513,        ///< IDI_ERROR (IDI_HAND)
    question = 32514,     ///< IDI_QUESTION
    warning = 32515,      ///< IDI_WARNING (IDI_EXCLAMATION)
    information = 32516,  ///< IDI_INFORMATION (IDI_ASTERISK)
    shield = 32518,       ///< IDI_SHIELD
};

/// Which system icon metric an icon is loaded at.
enum class IconSize {
    small,  ///< SM_CXSMICON x SM_CYSMICON: tray, title bar and small-icon views.
    large,  ///< SM_CXICON x SM_CYICON: Alt+Tab and large-icon views.
};

/// Loading icons as owned `wil::unique_hicon` handles.
namespace icon {

/// Loads a system icon as an icon this caller owns: the shared system icon
/// (`LoadImageW(..., LR_SHARED)`) copied at the requested size (`CopyImage`). Unlike
/// `LoadIconW(nullptr, IDI_*)`, the result is a private copy, so it is safe to hand to an
/// owner that destroys it, such as NotifyIcon.
[[nodiscard]] std::expected<wil::unique_hicon, std::error_code> load(SystemIcon icon,
                                                                     IconSize size);

/// Loads icon resource `resource_id` from `module` (an .exe or DLL; see module.hpp) as an
/// icon this caller owns (`LoadImageW` without `LR_SHARED`).
/// @return The icon, or the Win32 error; `ERROR_INVALID_HANDLE` for a null `module`, which
///         `LoadImageW` would otherwise read as a request for a system icon.
[[nodiscard]] std::expected<wil::unique_hicon, std::error_code> load(HMODULE module,
                                                                     WORD resource_id,
                                                                     IconSize size);

/// Loads icon resource `resource_id` from the running executable (module::current()).
[[nodiscard]] std::expected<wil::unique_hicon, std::error_code> load(WORD resource_id,
                                                                     IconSize size);

/// Loads an `.ico` file as an icon this caller owns, picking the image closest to `size`.
[[nodiscard]] std::expected<wil::unique_hicon, std::error_code> load(
    const std::filesystem::path& file, IconSize size);

}  // namespace icon

}  // namespace winwrap::user
