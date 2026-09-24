#pragma once

#include "winwrap/win.hpp"

#include <optional>

#include "winwrap/desktop/window/notification/command/protocol.hpp"

namespace winwrap::notification {

/// Reflects a child's WM_COMMAND notification from its parent back to the child.
/// The child receives a private message so its own notification mixins can handle it.
/// WM_NOTIFY has a separate protocol and can be added when it is implemented.
/// WindowCommand handles WM_COMMAND only when lparam is zero; this mixin handles it
/// only when lparam identifies a child control.
struct CommandReflection {
    [[nodiscard]] std::optional<LRESULT> handle_message(UINT msg, WPARAM wparam,
                                                        LPARAM lparam) const {
        if (msg == WM_COMMAND)
            if (auto* child = reinterpret_cast<HWND>(lparam)) {
                SendMessageW(child, wm_command_reflect, wparam, lparam);
                return 0;
            }
        return std::nullopt;
    }
};

}  // namespace winwrap::notification
