#include "winwrap/user/desktop/window/controls/button.hpp"

#include <catch2/catch_test_macros.hpp>

#include "winwrap/user/desktop/window/window.hpp"

namespace {
// Bare carrier for the notification::Click mixin, to exercise it without a live control.
struct ClickFixture : winwrap::user::notification::Click {};

// A host window that owns a button, so the full parent -> reflect -> control path
// can run for real.
struct ClickHost : winwrap::user::Window<ClickHost> {
    static constexpr const wchar_t* class_name = L"WinwrapClickHost";
    winwrap::user::CreationResult<winwrap::user::Button> button;

    void on_created() {
        button = winwrap::user::Button::create({.parent = hwnd(), .id = button_id, .text = L"OK"});
    }
    static constexpr UINT button_id = 1;
};
}  // namespace

TEST_CASE("notification::Click fires on_click on a reflected BN_CLICKED") {
    ClickFixture fix;
    bool clicked = false;
    fix.on_click = [&] { clicked = true; };

    const auto handled =
        fix.handle_message(winwrap::user::notification::wm_command_reflect, MAKEWPARAM(0, BN_CLICKED), 0);

    REQUIRE(clicked);
    REQUIRE(handled == 0);
}

TEST_CASE("notification::Click ignores messages that are not a reflected click") {
    ClickFixture fix;
    bool clicked = false;
    fix.on_click = [&] { clicked = true; };

    REQUIRE_FALSE(fix.handle_message(WM_PAINT, 0, 0).has_value());  // wrong message
    REQUIRE_FALSE(
        fix.handle_message(winwrap::user::notification::wm_command_reflect, MAKEWPARAM(0, BN_SETFOCUS), 0)
            .has_value());  // wrong notification
    REQUIRE_FALSE(clicked);
}

TEST_CASE("notification::Click with no handler swallows the click without crashing") {
    ClickFixture fix;  // on_click left unassigned
    REQUIRE(fix.handle_message(winwrap::user::notification::wm_command_reflect, MAKEWPARAM(0, BN_CLICKED),
                               0) == 0);
}

TEST_CASE("Button::create(cfg, handler) wires on_click in one call") {
    auto host = ClickHost::create();
    REQUIRE(host);

    bool clicked = false;
    auto made = winwrap::user::Button::create({.parent = host->hwnd(), .id = 2, .text = L"Go"},
                                        [&] { clicked = true; });
    REQUIRE(made);

    made->click();
    REQUIRE(clicked);
}

TEST_CASE("Button::click presses the native button and fires on_click") {
    auto host = ClickHost::create();
    REQUIRE(host);
    REQUIRE(host->button);

    int clicks = 0;
    host->button->on_click = [&] { ++clicks; };

    // BM_CLICK makes the native button send BN_CLICKED to its parent, which reflects it.
    host->button->click();
    host->button->click();

    CHECK(clicks == 2);
}

TEST_CASE("a control starts with the GUI font and set_font replaces it") {
    auto host = ClickHost::create();
    REQUIRE(host);
    REQUIRE(host->button);
    auto& button = *host->button;
    const auto system = static_cast<HFONT>(GetStockObject(SYSTEM_FONT));

    CHECK(button.font() == static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT)));
    button.set_font(system, false);
    CHECK(button.font() == system);
}

TEST_CASE("Button on_click fires through the parent window's reflection") {
    auto host = ClickHost::create();
    REQUIRE(host);
    REQUIRE(host->button);

    bool clicked = false;
    host->button->on_click = [&] { clicked = true; };

    // Windows delivers a click to the parent as WM_COMMAND; SendMessageW runs the
    // whole reflect -> notification::Click path synchronously, so no message pump is needed.
    winwrap::user::message::send(host->hwnd(), WM_COMMAND, MAKEWPARAM(ClickHost::button_id, BN_CLICKED),
                           reinterpret_cast<LPARAM>(host->button->hwnd()));

    REQUIRE(clicked);
}
