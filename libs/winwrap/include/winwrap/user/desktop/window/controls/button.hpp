#pragma once

#include "winwrap/user/mode.hpp"
#include "winwrap/user/win.hpp"

#include <functional>
#include <utility>

#include "winwrap/user/desktop/window/control.hpp"
#include "winwrap/user/desktop/window/notification/command/click.hpp"

namespace winwrap::user {

/// A native push button (the "BUTTON" system class) -- a concrete, ready-to-use thin
/// shell over the OS control, not a base to derive from. Create it with Button::create
/// (see Control::create for the creation / error contract), then assign a click handler.
/// on_click is a plain std::function, so it is reassignable at any time and defaults to
/// unset (an unhandled click does nothing):
///
///     button->on_click = [] { /* ... */ };
///
/// The click reaches the parent window as `BN_CLICKED`; the window's
/// notification::CommandReflection mixin bounces it back here, where the composed
/// notification::Click mixin fires `on_click` (which notification::Click provides -- see
/// <winwrap/user/desktop/window/notification/command/click.hpp>).
class Button final : public Control<Button, notification::Click> {
public:
    static constexpr const wchar_t* class_name = L"BUTTON";

    using Control::create;  // keep the handler-less create(cfg); see below re: name hiding

    /// Clicks the button as the user would (`BM_CLICK`): Windows presses and releases it,
    /// and the parent's reflected `BN_CLICKED` fires on_click before this returns.
    void click() const { send(BM_CLICK); }

    /// Creates the button and wires its click handler in one call -- a convenience for
    /// create(cfg) then assigning on_click. Both forms stay valid, and on_click remains
    /// reassignable afterwards.
    /// @param cfg       Control settings (parent, id, text, geometry, style).
    /// @param handler  Handler invoked on each click; moved into the button's on_click.
    /// @return          Stable owner of the live button, or the Win32 creation error.
    [[nodiscard]] static CreationResult<Button> create(const ControlConfig& cfg,
                                                       std::function<void()> handler) {
        auto button = Control::create(cfg);
        if (button)
            button->on_click = std::move(handler);
        return button;
    }
};

}  // namespace winwrap::user
