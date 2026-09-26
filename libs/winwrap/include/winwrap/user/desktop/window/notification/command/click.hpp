#pragma once

#include "winwrap/user/mode.hpp"
#include "winwrap/user/win.hpp"

#include <functional>
#include <optional>

#include "winwrap/user/desktop/window/notification/command/protocol.hpp"

namespace winwrap::user::notification {

/// CONTROL mixin: fires `on_click` on a reflected `BN_CLICKED`. Composed by any
/// control that emits `BN_CLICKED` -- buttons today, checkboxes / radio buttons later
/// (all "BUTTON"-class) -- via `class C : Control<C, notification::Click>`. It is tied to the
/// notification, not to any one control. The callback lives in the mixin, so a
/// control gains both the handler and its storage just by composing it; an unassigned
/// `on_click` is skipped, so a handler-less control is safe to click.
struct Click {
    std::function<void()> on_click;  ///< Assign your handler; unset = nothing happens.

    [[nodiscard]] std::optional<LRESULT> handle_message(UINT msg, WPARAM wparam, LPARAM) const {
        return handle_command(msg, wparam, BN_CLICKED, on_click);
    }
};

}  // namespace winwrap::user::notification
