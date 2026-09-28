#pragma once

#include "winwrap/detail/user_mode.hpp"

#include "winwrap/win.hpp"

/// Fonts for drawing and for windows' `WM_SETFONT` (set_font).
namespace winwrap::font {

/// The system's default font for user-interface objects such as menus and dialog boxes
/// (`GetStockObject(DEFAULT_GUI_FONT)`), the font native controls use in Winwrap.
/// @return A stock font, or null if Windows cannot supply it. It is borrowed and valid for the
///         life of the process: never delete it.
[[nodiscard]] inline HFONT default_gui() noexcept {
    return static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
}

}  // namespace winwrap::font
