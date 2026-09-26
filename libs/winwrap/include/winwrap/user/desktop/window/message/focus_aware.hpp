#pragma once

#include "winwrap/user/mode.hpp"
#include "winwrap/user/win.hpp"

#include <optional>

#include "winwrap/user/desktop/window/message/detail/hook_case.hpp"

namespace winwrap::user {

/// Routes `WM_SETFOCUS` / `WM_KILLFOCUS` to the final type's `on_focus(gained)` --
/// true on gain, false on loss.
struct FocusAware {
    std::optional<LRESULT> handle_message([[maybe_unused]] this auto& self, UINT msg, WPARAM,
                                          LPARAM) {
        switch (msg) {
            WW_CASE(WM_SETFOCUS, self.on_focus(true));
            WW_CASE(WM_KILLFOCUS, self.on_focus(false));
            default:
                break;
        }
        return std::nullopt;
    }
};

}  // namespace winwrap::user

#undef WW_CASE
