#pragma once

#include "winwrap/detail/user_mode.hpp"

#include "winwrap/win.hpp"

#include <optional>

#include "winwrap/desktop/window/message/detail/hook_case.hpp"

namespace winwrap {

/// Routes `WM_SIZE` to the final type's `on_size(width, height)` (client area,
/// from lparam).
struct SizeChange {
    std::optional<LRESULT> handle_message([[maybe_unused]] this auto& self, UINT msg, WPARAM,
                                          LPARAM lparam) {
        switch (msg) {
            WW_CASE(WM_SIZE, self.on_size(LOWORD(lparam), HIWORD(lparam)));
            default:
                break;
        }
        return std::nullopt;
    }
};

}  // namespace winwrap

#undef WW_CASE
