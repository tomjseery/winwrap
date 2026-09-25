#include <catch2/catch_test_macros.hpp>

#include "winwrap/desktop/window/controls/combobox.hpp"
#include "winwrap/desktop/window/window.hpp"

namespace {
struct ComboBoxHost : winwrap::Window<ComboBoxHost> {
    static constexpr const wchar_t* window_class_name = L"WinwrapComboBoxHost";
};

constexpr UINT combobox_id = 1;
}  // namespace

TEST_CASE("ComboBox selection round-trips through set_selection / selection") {
    ComboBoxHost host;
    REQUIRE(host.create());

    winwrap::ComboBox combo;
    REQUIRE(combo.create({.parent = host.hwnd(), .id = combobox_id}));

    combo.add_item(L"Alpha");
    combo.add_item(L"Beta");
    combo.add_item(L"Gamma");

    REQUIRE(combo.selection() == CB_ERR);  // nothing selected yet
    combo.set_selection(1);
    REQUIRE(combo.selection() == 1);
}

TEST_CASE("ComboBox fires on_selection_changed with the selected index") {
    ComboBoxHost host;
    REQUIRE(host.create());

    winwrap::ComboBox combo;
    REQUIRE(combo.create({.parent = host.hwnd(), .id = combobox_id}));

    combo.add_item(L"Alpha");
    combo.add_item(L"Beta");
    combo.add_item(L"Gamma");
    combo.set_selection(2);  // the payload the mixin will read back via CB_GETCURSEL

    int got = -1;
    combo.on_selection_changed = [&](int index) { got = index; };

    winwrap::message::send(host.hwnd(), WM_COMMAND, MAKEWPARAM(combobox_id, CBN_SELCHANGE),
                           reinterpret_cast<LPARAM>(combo.hwnd()));

    REQUIRE(got == 2);
}
