#pragma once

#include "winwrap/win.hpp"

#include <optional>

#include "winwrap/desktop/window/detail/hook_case.hpp"

namespace winwrap {

/// Routes `WM_SETFOCUS` / `WM_KILLFOCUS` to the final type's `on_focus(gained)` --
/// true on gain, false on loss.
struct FocusMessages {
    std::optional<LRESULT> handle_message([[maybe_unused]] this auto& self, UINT msg, WPARAM,
                                          LPARAM) {
        switch (msg) {
            WINWRAP_HOOK_CASE(WM_SETFOCUS, self.on_focus(true));
            WINWRAP_HOOK_CASE(WM_KILLFOCUS, self.on_focus(false));
            default:
                break;
        }
        return std::nullopt;
    }
};

}  // namespace winwrap
