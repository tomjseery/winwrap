#pragma once

#include "winwrap/user/mode.hpp"
#include "winwrap/user/win.hpp"

#include <optional>

#include "winwrap/user/desktop/window/message/detail/hook_case.hpp"

namespace winwrap::user {

/// Routes `WM_PAINT` to the final type's `on_paint()` when defined.
struct Paintable {
    std::optional<LRESULT> handle_message([[maybe_unused]] this auto& self, UINT msg, WPARAM,
                                          LPARAM) {
        switch (msg) {
            WW_CASE(WM_PAINT, self.on_paint());
            default:
                break;
        }
        return std::nullopt;
    }
};

}  // namespace winwrap::user

#undef WW_CASE
