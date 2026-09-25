#include <catch2/catch_test_macros.hpp>

#include "winwrap/desktop/window/window.hpp"

namespace {
// A minimal derived window used only as a compile-time fixture: forming
// &TestWindow::create (in the test below) forces the whole Window<T> chain to
// instantiate -- create -> create_window -> window_proc -> route_message -- so
// the build compiles and links it. Two hooks are defined so both branches of the
// if-constexpr detection get checked. Running a window for real needs a message pump.
struct TestWindow : winwrap::Window<TestWindow> {
    static constexpr const wchar_t* window_class_name = L"WinwrapTestWindow";
    void on_paint() {}
    void on_command(UINT /*id*/) {}
};

struct RoutedWindow : winwrap::Window<RoutedWindow> {
    static constexpr const wchar_t* window_class_name = L"WinwrapRoutedWindow";
    bool delegated_to_default{};

    LRESULT route_message(UINT msg, WPARAM wparam, LPARAM lparam) {
        if (msg == WM_SETTEXT) {
            delegated_to_default = true;
            return default_proc(msg, wparam, lparam);
        }
        return Window::route_message(msg, wparam, lparam);
    }
};
}  // namespace

TEST_CASE("Window<T> compiles and links") {
    (void)&TestWindow::create;  // forms the address -> forces the template chain to compile
    REQUIRE(true);
}

TEST_CASE("Window custom routing can delegate to its native default procedure") {
    RoutedWindow window;
    REQUIRE(window.create({.parent = HWND_MESSAGE}));

    constexpr const wchar_t* text{L"Routed by DefWindowProcW"};
    const auto result =
        winwrap::message::send(window.hwnd(), WM_SETTEXT, 0, reinterpret_cast<LPARAM>(text));

    REQUIRE(window.delegated_to_default);
    CHECK(result != FALSE);
    CHECK(window.text() == text);
}

TEST_CASE("Window rejects a second create while its native window is live") {
    RoutedWindow window;
    REQUIRE(window.create({.parent = HWND_MESSAGE}));

    const auto duplicate = window.create({.parent = HWND_MESSAGE});

    REQUIRE_FALSE(duplicate);
    CHECK(duplicate.error().value() == ERROR_ALREADY_EXISTS);
}

TEST_CASE("Window can create again after its native window is destroyed") {
    RoutedWindow window;
    REQUIRE(window.create({.parent = HWND_MESSAGE}));
    window.send(WM_CLOSE);
    REQUIRE(window.hwnd() == nullptr);

    CHECK(window.create({.parent = HWND_MESSAGE}));
}

TEST_CASE("Window can retry after native creation fails") {
    RoutedWindow window;

    const auto failed = window.create({.style = WS_CHILD});

    REQUIRE_FALSE(failed);
    REQUIRE(window.hwnd() == nullptr);
    CHECK(window.create({.parent = HWND_MESSAGE}));
}
