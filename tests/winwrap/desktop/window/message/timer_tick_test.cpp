#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <vector>

#include "winwrap/desktop/message_loop.hpp"
#include "winwrap/desktop/window/window.hpp"

using namespace std::chrono_literals;

namespace {

struct TickingWindow : winwrap::Window<TickingWindow> {
    static constexpr const wchar_t* class_name = L"WinwrapTimerTickTest";
    std::vector<UINT_PTR> ticks;

    void on_timer(UINT_PTR id) {
        ticks.push_back(id);
        if (ticks.size() == 2) {
            CHECK(stop_timer(id));  // never REQUIRE: a throw must not cross the WndProc
            winwrap::message_loop::quit();
        }
    }
};

struct QuietWindow : winwrap::Window<QuietWindow> {
    static constexpr const wchar_t* class_name = L"WinwrapTimerTickQuietTest";
};

bool timer_proc_called{};
void CALLBACK record_timer_proc(HWND hwnd, UINT, UINT_PTR id, DWORD) {
    timer_proc_called = true;
    KillTimer(hwnd, id);
    winwrap::message_loop::quit();
}

}  // namespace

TEST_CASE("start_timer delivers each tick to on_timer until stop_timer") {
    auto window = TickingWindow::create({.parent = HWND_MESSAGE});
    REQUIRE(window);

    REQUIRE(window->start_timer(7, 10ms));

    CHECK(winwrap::message_loop::run() == 0);
    CHECK(window->ticks == std::vector<UINT_PTR>{7, 7});
    CHECK_FALSE(window->stop_timer(7));  // already stopped
}

TEST_CASE("a raw SetTimer callback keeps its ticks away from on_timer") {
    auto window = TickingWindow::create({.parent = HWND_MESSAGE});
    REQUIRE(window);
    timer_proc_called = false;

    REQUIRE(SetTimer(window->hwnd(), 3, 10, &record_timer_proc) != 0);
    CHECK(winwrap::message_loop::run() == 0);
    CHECK(timer_proc_called);

    // A WM_TIMER that names a procedure is not claimed as an on_timer tick either.
    winwrap::message::send(window->hwnd(), WM_TIMER, 3,
                           reinterpret_cast<LPARAM>(&record_timer_proc));
    CHECK(window->ticks.empty());
}

TEST_CASE("start_timer rejects an interval Windows cannot represent") {
    auto window = QuietWindow::create({.parent = HWND_MESSAGE});
    REQUIRE(window);

    const auto negative = window->start_timer(1, -1ms);
    const auto too_long =
        window->start_timer(1, std::chrono::milliseconds{USER_TIMER_MAXIMUM} + 1ms);

    REQUIRE_FALSE(negative);
    REQUIRE_FALSE(too_long);
    CHECK(negative.error().value() == ERROR_INVALID_PARAMETER);
    CHECK(too_long.error().value() == ERROR_INVALID_PARAMETER);
}

TEST_CASE("stop_timer reports a timer that is not running") {
    auto window = QuietWindow::create({.parent = HWND_MESSAGE});
    REQUIRE(window);

    CHECK_FALSE(window->stop_timer(42));
}

struct ClosingTimerWindow : winwrap::Window<ClosingTimerWindow> {
    static constexpr const wchar_t* class_name = L"WinwrapTimerTickClosingTest";
    void on_destroy() { winwrap::message_loop::quit(); }
};

TEST_CASE("timer operations on a destroyed window never reach thread timers") {
    auto window = ClosingTimerWindow::create({.parent = HWND_MESSAGE});
    REQUIRE(window);
    REQUIRE(window->request_close());
    REQUIRE(winwrap::message_loop::run() == 0);

    const auto started = window->start_timer(1, 10ms);
    const auto stopped = window->stop_timer(1);

    REQUIRE_FALSE(started);
    REQUIRE_FALSE(stopped);
    CHECK(started.error().value() == ERROR_INVALID_WINDOW_HANDLE);
    CHECK(stopped.error().value() == ERROR_INVALID_WINDOW_HANDLE);
}
