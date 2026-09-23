#pragma once

#include "winwrap/win.hpp"

#include <functional>
#include <optional>

namespace winwrap::notification {

/// App-private message used to reflect a control's `WM_COMMAND` from the parent back
/// to the control: Reflection sends it, the control's mixins (e.g. Click)
/// receive it. It sits in the `WM_APP` range reserved for application messages.
/// Other subclasses of the same control can still choose the same identifier.
inline constexpr UINT wm_command_reflect = WM_APP + 0x1c00;

/// The shared "reflected `WM_COMMAND` -> callback" match-and-fire, written once for
/// every control notification mixin to build on. If `msg` is the reflected command
/// and its notification code is `code`, fire `handler` (when assigned) and report the
/// message handled; otherwise pass. (See the mixins in <winwrap/window/notification/>.)
/// @param code     The control notification code to match (`BN_CLICKED`, `EN_CHANGE`, ...).
/// @param handler  The callback to fire on a match; an unassigned one is skipped.
/// @return         0 (handled) on a match, else std::nullopt to keep looking.
inline std::optional<LRESULT> handle(UINT msg, WPARAM wparam, WORD code,
                                    const std::function<void()>& handler) {
    if (msg == wm_command_reflect && HIWORD(wparam) == code) {
        if (handler)
            handler();
        return 0;
    }
    return std::nullopt;
}

/// Payload-carrying overload: same match, but the notification's data isn't in the
/// message -- it lives in the control and must be queried (e.g. `CBN_SELCHANGE` -> the
/// selected index via `CB_GETCURSEL`). `fetch` is run **only after** the match, so the
/// control is touched solely when the notification actually fired; its result is the
/// argument passed to `handler`.
/// @param handler  A callback taking the payload (e.g. std::function<void(int)>).
/// @param fetch    A nullary callable that produces the payload; evaluated lazily.
template <typename Handler, typename Fetch>
std::optional<LRESULT> handle(UINT msg, WPARAM wparam, WORD code,
                              const Handler& handler, Fetch fetch) {
    if (msg == wm_command_reflect && HIWORD(wparam) == code) {
        if (handler)
            handler(fetch());
        return 0;
    }
    return std::nullopt;
}

}  // namespace winwrap::notification
