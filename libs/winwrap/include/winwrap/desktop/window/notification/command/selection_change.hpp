#pragma once

#include "winwrap/win.hpp"

#include <functional>
#include <optional>

#include "winwrap/desktop/window/notification/command/protocol.hpp"

namespace winwrap::notification {

/// CONTROL mixin: fires `on_selection_changed(index)` on a reflected `CBN_SELCHANGE`
/// -- the user picked a different combo-box item. Composed by any control that emits
/// `CBN_SELCHANGE` (the "COMBOBOX" class) via `class C : Control<C, notification::SelectionChange>`.
///
/// Unlike Click / TextChange, this carries a payload: the selected index is not
/// in the notification, so the mixin queries the control (`CB_GETCURSEL`) once the
/// match is confirmed -- which is why this handle_message takes the deduced object
/// (`this auto& self`, to reach `hwnd()`), where the void mixins need no object at all.
/// The index is `CB_ERR` (-1) when the selection was cleared. An unassigned
/// `on_selection_changed` is skipped.
struct SelectionChange {
    std::function<void(int)> on_selection_changed;  ///< Assign your handler; gets the new index.

    std::optional<LRESULT> handle_message(this auto& self, UINT msg, WPARAM wparam, LPARAM) {
        return handle_command(msg, wparam, CBN_SELCHANGE, self.on_selection_changed, [&self] {
            return static_cast<int>(self.send(CB_GETCURSEL));
        });
    }
};

}  // namespace winwrap::notification
