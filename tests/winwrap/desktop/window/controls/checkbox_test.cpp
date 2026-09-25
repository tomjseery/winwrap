#include <catch2/catch_test_macros.hpp>

#include "winwrap/desktop/window/controls/checkbox.hpp"
#include "winwrap/desktop/window/window.hpp"

namespace {
// A bare host window to parent the checkbox, so the parent -> reflect -> control path
// can run for real.
struct CheckboxHost : winwrap::Window<CheckboxHost> {
    static constexpr const wchar_t* window_class_name = L"WinwrapCheckboxHost";
};

constexpr UINT checkbox_id = 1;
}  // namespace

TEST_CASE("Checkbox::click toggles the box and fires on_click through reflection") {
    CheckboxHost host;
    REQUIRE(host.create());

    winwrap::Checkbox checkbox;
    REQUIRE(checkbox.create({.parent = host.hwnd(), .id = checkbox_id, .text = L"Enable"}));

    bool clicked = false;
    checkbox.on_click = [&] { clicked = true; };

    checkbox.click();

    REQUIRE(clicked);
    CHECK(checkbox.checked());  // the native control toggled itself
    checkbox.click();
    CHECK_FALSE(checkbox.checked());
}

TEST_CASE("Checkbox check state round-trips through set_checked / checked") {
    CheckboxHost host;
    REQUIRE(host.create());

    winwrap::Checkbox checkbox;
    REQUIRE(checkbox.create({.parent = host.hwnd(), .id = checkbox_id}));

    REQUIRE_FALSE(checkbox.checked());
    checkbox.set_checked(true);
    REQUIRE(checkbox.checked());
    checkbox.set_checked(false);
    REQUIRE_FALSE(checkbox.checked());
}
