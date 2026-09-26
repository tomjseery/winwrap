#pragma once

#include "winwrap/user/mode.hpp"
#include "winwrap/user/win.hpp"

#include <optional>

#include "winwrap/user/desktop/window/message/detail/hook_case.hpp"

namespace winwrap::user {

/// Routes `WM_KEYDOWN` to the final type's `on_key_down(vk)` (the virtual-key code).
struct KeyboardInput {
    std::optional<LRESULT> handle_message([[maybe_unused]] this auto& self, UINT msg, WPARAM wparam,
                                          LPARAM) {
        switch (msg) {
            WW_CASE(WM_KEYDOWN, self.on_key_down(static_cast<WORD>(wparam)));
            default:
                break;
        }
        return std::nullopt;
    }
};

}  // namespace winwrap::user

#undef WW_CASE
