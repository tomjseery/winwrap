#include "winwrap/user/desktop/window/controls/checkbox.hpp"

#include <catch2/catch_test_macros.hpp>

#include "winwrap/user/desktop/window/window.hpp"

namespace {
// A bare host window to parent the checkbox, so the parent -> reflect -> control path
// can run for real.
struct CheckboxHost : winwrap::user::Window<CheckboxHost> {
    static constexpr const wchar_t* class_name = L"WinwrapCheckboxHost";
};

constexpr UINT checkbox_id = 1;
}  // namespace

TEST_CASE("Checkbox::click toggles the box and fires on_click through reflection") {
    auto host = CheckboxHost::create();
    REQUIRE(host);

    auto checkbox =
        winwrap::user::Checkbox::create({.parent = host->hwnd(), .id = checkbox_id, .text = L"Enable"});
    REQUIRE(checkbox);

    bool clicked = false;
    checkbox->on_click = [&] { clicked = true; };

    checkbox->click();

    REQUIRE(clicked);
    CHECK(checkbox->checked());  // the native control toggled itself
    checkbox->click();
    CHECK_FALSE(checkbox->checked());
}

TEST_CASE("Checkbox check state round-trips through set_checked / checked") {
    auto host = CheckboxHost::create();
    REQUIRE(host);

    auto checkbox = winwrap::user::Checkbox::create({.parent = host->hwnd(), .id = checkbox_id});
    REQUIRE(checkbox);

    REQUIRE_FALSE(checkbox->checked());
    checkbox->set_checked(true);
    REQUIRE(checkbox->checked());
    checkbox->set_checked(false);
    REQUIRE_FALSE(checkbox->checked());
}
