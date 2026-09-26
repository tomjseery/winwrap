#pragma once

#include "winwrap/user/mode.hpp"
#include "winwrap/user/win.hpp"

#include <optional>

namespace winwrap::user {

/// Window message behavior that routes a menu or accelerator command (lparam == 0) to the final
/// type's `on_command(id)` (low word of wparam). Control notifications (lparam != 0)
/// are not commands the window handles directly -- notification::CommandReflection bounces those
/// home. WindowCommand and notification::CommandReflection split `WM_COMMAND` between them by
/// lparam (== 0 here,
/// != 0 there). Dispatch is first-match-wins, so if these conditions ever overlap --
/// or another mixin matches `WM_COMMAND` -- one handler silently steals the other's
/// messages. Keep the split exact.
struct WindowCommand {
    std::optional<LRESULT> handle_message([[maybe_unused]] this auto& self, UINT msg, WPARAM wparam,
                                          LPARAM lparam) {
        if (msg == WM_COMMAND && lparam == 0)
            if constexpr (requires { self.on_command(LOWORD(wparam)); }) {
                self.on_command(LOWORD(wparam));
                return 0;
            }
        return std::nullopt;
    }
};

}  // namespace winwrap::user
