#pragma once

#include "winwrap/user/mode.hpp"
#include "winwrap/user/win.hpp"

#include <functional>
#include <optional>

#include "winwrap/user/desktop/window/notification/command/protocol.hpp"

namespace winwrap::user::notification {

/// CONTROL mixin: fires `on_text_changed` on a reflected `EN_CHANGE` -- the edit's
/// contents changed. Composed by any control that emits `EN_CHANGE` (the "EDIT" class)
/// via `class C : Control<C, notification::TextChange>`. The callback is `void()`, not
/// `void(text)`: `EN_CHANGE` carries no text, and reading it on every keystroke would
/// allocate needlessly -- pull the current value with `control.text()` inside the
/// handler when you actually want it. An unassigned `on_text_changed` is skipped.
struct TextChange {
    std::function<void()> on_text_changed;  ///< Assign your handler; unset = nothing happens.

    [[nodiscard]] std::optional<LRESULT> handle_message(UINT msg, WPARAM wparam, LPARAM) const {
        return handle_command(msg, wparam, EN_CHANGE, on_text_changed);
    }
};

}  // namespace winwrap::user::notification
