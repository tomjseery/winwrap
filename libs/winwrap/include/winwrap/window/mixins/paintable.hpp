#pragma once

#include "winwrap/win.hpp"

#include <optional>

#include "winwrap/window/detail/hook_case.hpp"

namespace winwrap {

/// Routes `WM_PAINT` to the final type's `on_paint()` when defined.
struct Paintable {
    std::optional<LRESULT> handle_message([[maybe_unused]] this auto& self, UINT msg, WPARAM,
                                          LPARAM) {
        switch (msg) {
            WINWRAP_HOOK_CASE(WM_PAINT, self.on_paint());
            default:
                break;
        }
        return std::nullopt;
    }
};

}  // namespace winwrap
