#pragma once

#include "winwrap/win.hpp"

#include "winwrap/desktop/window/notification/command/selection_change.hpp"
#include "winwrap/desktop/window/control.hpp"

namespace winwrap {

/// A native drop-down list (the "COMBOBOX" system class, CBS_DROPDOWNLIST: pick from the
/// list, no free typing). It composes notification::SelectionChange, so `on_selection_changed(index)`
/// fires when the user picks a different item (the reflected `CBN_SELCHANGE`). Populate it
/// with `add_item`, and read or set the current pick with `selection()` / `set_selection()`.
class ComboBox final : public Control<ComboBox, notification::SelectionChange> {
public:
    static constexpr const wchar_t* control_class = L"COMBOBOX";
    static constexpr DWORD default_style = CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP;

    /// Appends an item to the end of the list (CB_ADDSTRING).
    void add_item(const wchar_t* text) { send(CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text)); }

    /// The index of the selected item, or CB_ERR (-1) when nothing is selected.
    [[nodiscard]] int selection() const { return static_cast<int>(send(CB_GETCURSEL)); }

    /// Selects the item at `index` (CB_SETCURSEL); pass -1 to clear the selection.
    void set_selection(int index) { send(CB_SETCURSEL, static_cast<WPARAM>(index)); }
};

}  // namespace winwrap
