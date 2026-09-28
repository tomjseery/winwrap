#pragma once

#include "winwrap/detail/user_mode.hpp"

#include "winwrap/win.hpp"

#include "winwrap/desktop/window/control.hpp"
#include "winwrap/desktop/window/notification/command/click.hpp"

namespace winwrap {

/// A native check box -- the "BUTTON" system class with the BS_AUTOCHECKBOX style, which
/// makes Windows toggle the check state on each click for you. It reuses the very same
/// notification::Click mixin as Button (both are "BUTTON"-class controls that emit `BN_CLICKED`),
/// so `on_click` fires on every toggle; read the resulting state with `checked()`.
class Checkbox final : public Control<Checkbox, notification::Click> {
public:
    static constexpr const wchar_t* class_name = L"BUTTON";
    static constexpr DWORD default_style = BS_AUTOCHECKBOX | WS_TABSTOP;

    /// Clicks the box as the user would (`BM_CLICK`): Windows toggles it, and the parent's
    /// reflected `BN_CLICKED` fires on_click before this returns.
    void click() const { send(BM_CLICK); }

    /// Whether the box is currently ticked (`BM_GETCHECK`).
    [[nodiscard]] bool checked() const { return send(BM_GETCHECK) == BST_CHECKED; }

    /// Ticks or unticks the box (`BM_SETCHECK`).
    void set_checked(bool checked) { send(BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED); }
};

}  // namespace winwrap
