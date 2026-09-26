#include "winwrap/desktop/notify_icon.hpp"

#include <catch2/catch_test_macros.hpp>

#include "winwrap/desktop/icon.hpp"
#include "winwrap/desktop/window/window.hpp"

namespace {

struct TrayOwner : winwrap::Window<TrayOwner> {
    static constexpr const wchar_t* class_name = L"WinwrapNotifyIconTest";
};

constexpr UINT tray_callback{WM_APP + 1};

// Needs a running shell: the icon briefly appears in the notification area.
winwrap::NotifyIcon make_tray_icon(HWND owner) {
    auto icon = winwrap::icon::load(winwrap::SystemIcon::application, winwrap::IconSize::small);
    REQUIRE(icon);
    auto tray = winwrap::NotifyIcon::create({.owner = owner,
                                             .callback_msg = tray_callback,
                                             .id = 1,
                                             .icon = icon->release(),
                                             .tooltip = L"winwrap test"});
    REQUIRE(tray);
    return std::move(*tray);
}

}  // namespace

TEST_CASE("set_icon replaces the tray icon") {
    auto owner = TrayOwner::create({.parent = HWND_MESSAGE});
    REQUIRE(owner);
    auto tray = make_tray_icon(owner->hwnd());
    auto replacement = winwrap::icon::load(winwrap::SystemIcon::warning, winwrap::IconSize::small);
    REQUIRE(replacement);

    CHECK(tray.set_icon(std::move(*replacement)));
}

TEST_CASE("set_icon reports a registration the shell does not know") {
    auto owner = TrayOwner::create({.parent = HWND_MESSAGE});
    REQUIRE(owner);
    auto tray = make_tray_icon(owner->hwnd());
    auto moved_to = std::move(tray);  // the moved-from icon no longer owns a registration
    auto replacement = winwrap::icon::load(winwrap::SystemIcon::warning, winwrap::IconSize::small);
    REQUIRE(replacement);

    const auto result = tray.set_icon(std::move(*replacement));  // NOLINT(bugprone-use-after-move)
    REQUIRE_FALSE(result);
    CHECK(result.error().value() == ERROR_INVALID_WINDOW_HANDLE);
}
