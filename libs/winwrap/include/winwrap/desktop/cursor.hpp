#pragma once

#include "winwrap/detail/user_mode.hpp"

#include "winwrap/win.hpp"

#include <expected>
#include <system_error>

#include "winwrap/error.hpp"

/// The mouse cursor.
namespace winwrap::cursor {

/// The cursor's current position in screen coordinates (GetCursorPos).
/// @return The position, or the Win32 error, e.g. when the calling thread's desktop is not
///         the input desktop.
[[nodiscard]] inline std::expected<POINT, std::error_code> position() {
    POINT point{};
    return error::nonzero_or_last(GetCursorPos(&point)).transform([&] { return point; });
}

}  // namespace winwrap::cursor
