#include <catch2/catch_test_macros.hpp>

#include <vector>

#include "winwrap/desktop/message_loop.hpp"
#include "winwrap/desktop/window/messaging.hpp"
#include "winwrap/desktop/window/window.hpp"

namespace {

constexpr UINT app_message{WM_APP + 7};

struct MessageWindow : winwrap::Window<MessageWindow> {
    static constexpr const wchar_t* window_class_name = L"WinwrapMessagingTest";
    std::vector<WPARAM> received;

    LRESULT route_message(UINT msg, WPARAM wparam, LPARAM lparam) {
        if (msg == app_message) {
            received.push_back(wparam);
            if (lparam != 0)
                winwrap::quit();
            return static_cast<LRESULT>(wparam) * 2;
        }
        return Window::route_message(msg, wparam, lparam);
    }
};

}  // namespace

TEST_CASE("send_message runs the window procedure and returns its result") {
    auto window = MessageWindow::create({.parent = HWND_MESSAGE});
    REQUIRE(window);

    CHECK(winwrap::send_message((*window)->hwnd(), app_message, 21) == 42);
    CHECK((*window)->send(app_message, 5) == 10);
    CHECK((*window)->received == std::vector<WPARAM>{21, 5});
}

TEST_CASE("post_message queues the message for the message loop") {
    auto window = MessageWindow::create({.parent = HWND_MESSAGE});
    REQUIRE(window);

    REQUIRE(winwrap::post_message((*window)->hwnd(), app_message, 1));
    REQUIRE((*window)->post(app_message, 2, 1));  // lparam 1: quit after this one
    CHECK((*window)->received.empty());           // nothing runs until the loop does

    CHECK(winwrap::run() == 0);
    CHECK((*window)->received == std::vector<WPARAM>{1, 2});
}

TEST_CASE("post_message rejects a null window instead of posting to the thread") {
    const auto posted = winwrap::post_message(nullptr, app_message);

    REQUIRE_FALSE(posted);
    CHECK(posted.error().value() == ERROR_INVALID_WINDOW_HANDLE);
    MSG msg{};
    CHECK_FALSE(PeekMessageW(&msg, reinterpret_cast<HWND>(-1), app_message, app_message, PM_REMOVE));
}

TEST_CASE("send_message to a destroyed window returns 0") {
    HWND gone{};
    {
        auto window = MessageWindow::create({.parent = HWND_MESSAGE});
        REQUIRE(window);
        gone = (*window)->hwnd();
    }

    CHECK(winwrap::send_message(gone, app_message, 21) == 0);
}
