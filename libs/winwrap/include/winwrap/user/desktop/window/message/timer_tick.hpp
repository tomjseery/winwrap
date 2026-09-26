#pragma once

#include "winwrap/user/mode.hpp"
#include "winwrap/user/win.hpp"

#include <optional>

namespace winwrap::user {

/// Routes `WM_TIMER` to the final type's `on_timer(id)`, where `id` is the timer id
/// passed to `Window::start_timer`.
///
/// A timer started with a `TIMERPROC` (a raw `SetTimer` callback) belongs to that
/// procedure: the message loop's `DispatchMessageW` calls it directly, and a `WM_TIMER`
/// that carries one in lparam is left unclaimed rather than routed to `on_timer`.
struct TimerTick {
    std::optional<LRESULT> handle_message([[maybe_unused]] this auto& self, UINT msg,
                                          [[maybe_unused]] WPARAM wparam, LPARAM lparam) {
        if constexpr (requires { self.on_timer(UINT_PTR{}); }) {
            if (msg == WM_TIMER && lparam == 0) {
                self.on_timer(static_cast<UINT_PTR>(wparam));
                return 0;
            }
        }
        return std::nullopt;
    }
};

}  // namespace winwrap::user
