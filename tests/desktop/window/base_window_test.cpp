#include <catch2/catch_test_macros.hpp>
#include <chrono>

#include "winwrap/desktop/font.hpp"
#include "winwrap/desktop/message_loop.hpp"
#include "winwrap/desktop/window/window.hpp"
#include "winwrap/module.hpp"

namespace {

// A real top-level window, created hidden and far off-screen so the tests never flash it.
struct PlainWindow : winwrap::Window<PlainWindow> {
    static constexpr const wchar_t* class_name = L"WinwrapBaseWindowTest";
};

struct ClosingWindow : winwrap::Window<ClosingWindow> {
    static constexpr const wchar_t* class_name = L"WinwrapBaseWindowClosingTest";
    void on_destroy() { winwrap::message_loop::quit(); }
};

constexpr int off_screen{-20000};

auto make_plain_window(DWORD ex_style = 0) {
    auto window = PlainWindow::create({.style = WS_OVERLAPPEDWINDOW,
                                       .ex_style = ex_style,
                                       .x = off_screen,
                                       .y = off_screen,
                                       .width = 320,
                                       .height = 240});
    REQUIRE(window);
    return window;
}

}  // namespace

TEST_CASE("move changes the position and keeps the size") {
    auto window = make_plain_window();
    const auto before = window->window_rect();
    REQUIRE(before);

    REQUIRE(window->move(off_screen + 10, off_screen + 20));

    const auto after = window->window_rect();
    REQUIRE(after);
    CHECK(after->left == off_screen + 10);
    CHECK(after->top == off_screen + 20);
    CHECK(after->right - after->left == before->right - before->left);
    CHECK(after->bottom - after->top == before->bottom - before->top);
}

TEST_CASE("resize changes the whole-window size and keeps the position") {
    auto window = make_plain_window();

    REQUIRE(window->resize(400, 300));

    const auto bounds = window->window_rect();
    REQUIRE(bounds);
    CHECK(bounds->left == off_screen);
    CHECK(bounds->top == off_screen);
    CHECK(bounds->right - bounds->left == 400);
    CHECK(bounds->bottom - bounds->top == 300);
}

TEST_CASE("client_rect starts at the origin and excludes the frame") {
    auto window = make_plain_window();

    const auto client = window->client_rect();
    const auto bounds = window->window_rect();
    REQUIRE(client);
    REQUIRE(bounds);
    CHECK(client->left == 0);
    CHECK(client->top == 0);
    CHECK(client->right > 0);
    CHECK(client->right < bounds->right - bounds->left);
    CHECK(client->bottom < bounds->bottom - bounds->top);
}

TEST_CASE("show and hide toggle visibility") {
    auto window = make_plain_window();
    REQUIRE_FALSE(window->is_visible());

    window->show(SW_SHOWNOACTIVATE);
    CHECK(window->is_visible());

    window->hide();
    CHECK_FALSE(window->is_visible());
}

TEST_CASE("enable and is_enabled agree") {
    auto window = make_plain_window();
    REQUIRE(window->is_enabled());

    window->enable(false);
    CHECK_FALSE(window->is_enabled());

    window->enable(true);
    CHECK(window->is_enabled());
}

TEST_CASE("focus gives the window keyboard focus") {
    auto window = make_plain_window();

    REQUIRE(window->focus());

    CHECK(window->has_focus());
    CHECK(GetFocus() == window->hwnd());
}

TEST_CASE("a plain window stores no font") {
    auto window = make_plain_window();

    window->set_font(winwrap::font::default_gui(), false);

    CHECK(window->font() == nullptr);  // DefWindowProcW ignores WM_SETFONT
}

TEST_CASE("style and ex_style report the window's style bits") {
    auto window = make_plain_window(WS_EX_TOOLWINDOW);

    const auto style = window->style();
    const auto ex_style = window->ex_style();
    REQUIRE(style);
    REQUIRE(ex_style);
    CHECK((*style & WS_CAPTION) == WS_CAPTION);
    CHECK((*ex_style & WS_EX_TOOLWINDOW) != 0);
}

TEST_CASE("invalidate marks the client area for repainting") {
    auto window = make_plain_window();
    window->show(SW_SHOWNOACTIVATE);  // a hidden window keeps no update region
    ValidateRect(window->hwnd(), nullptr);
    REQUIRE_FALSE(GetUpdateRect(window->hwnd(), nullptr, FALSE));

    window->invalidate();

    CHECK(GetUpdateRect(window->hwnd(), nullptr, FALSE));
}

TEST_CASE("request_close closes the window from the message loop") {
    auto window = ClosingWindow::create({.parent = HWND_MESSAGE});
    REQUIRE(window);

    REQUIRE(window->request_close());
    CHECK(window->hwnd() != nullptr);  // nothing happens until the loop runs

    CHECK(winwrap::message_loop::run() == 0);
    CHECK(window->hwnd() == nullptr);
}

TEST_CASE("operations on a destroyed window report the Win32 error") {
    auto window = ClosingWindow::create({.parent = HWND_MESSAGE});
    REQUIRE(window);
    REQUIRE(window->request_close());
    REQUIRE(winwrap::message_loop::run() == 0);

    const auto client = window->client_rect();
    const auto style = window->style();
    REQUIRE_FALSE(client);
    REQUIRE_FALSE(style);
    CHECK(client.error().value() == ERROR_INVALID_WINDOW_HANDLE);
    CHECK(style.error().value() == ERROR_INVALID_WINDOW_HANDLE);
    CHECK(window->move(0, 0).error().value() == ERROR_INVALID_WINDOW_HANDLE);
    CHECK(window->request_close().error().value() == ERROR_INVALID_WINDOW_HANDLE);
    CHECK_FALSE(window->has_focus());
}

TEST_CASE("window operations act on a raw HWND without a wrapper") {
    auto window = make_plain_window();
    const HWND raw = window->hwnd();

    winwrap::window::set_text(raw, L"raw");
    CHECK(winwrap::window::text(raw) == L"raw");
    CHECK(window->text() == L"raw");  // the member reads the same native state

    REQUIRE(winwrap::window::move(raw, off_screen + 5, off_screen + 5));
    const auto bounds = winwrap::window::window_rect(raw);
    REQUIRE(bounds);
    CHECK(bounds->left == off_screen + 5);
}

TEST_CASE("long_ptr reads back what set_long_ptr stored and returns the previous value") {
    // A plain system-class window: nothing else uses its GWLP_USERDATA slot.
    wil::unique_hwnd raw{CreateWindowExW(0, L"STATIC", L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr,
                                         winwrap::module::current(), nullptr)};
    REQUIRE(raw);

    const auto previous = winwrap::window::set_long_ptr(raw.get(), GWLP_USERDATA, 1234);
    REQUIRE(previous);
    CHECK(*previous == 0);  // zero is a legitimate previous value, not a failure

    const auto current = winwrap::window::long_ptr(raw.get(), GWLP_USERDATA);
    REQUIRE(current);
    CHECK(*current == 1234);
}

TEST_CASE("destroy destroys a raw window and reports a dead one") {
    auto raw = CreateWindowExW(0, L"STATIC", L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr,
                               winwrap::module::current(), nullptr);
    REQUIRE(raw);

    REQUIRE(winwrap::window::destroy(raw));
    CHECK_FALSE(IsWindow(raw));

    const auto again = winwrap::window::destroy(raw);
    REQUIRE_FALSE(again);
    CHECK(again.error().value() == ERROR_INVALID_WINDOW_HANDLE);
}

namespace {
UINT_PTR subclass_seen_id{};

LRESULT CALLBACK recording_subclass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR id,
                                    DWORD_PTR data) {
    if (msg == WM_APP + 7)
        *reinterpret_cast<UINT_PTR*>(data) = id;
    return winwrap::window::default_subclass_proc(hwnd, msg, wparam, lparam);
}
}  // namespace

TEST_CASE("subclass hooks a raw window until remove_subclass") {
    wil::unique_hwnd raw{CreateWindowExW(0, L"STATIC", L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr,
                                         winwrap::module::current(), nullptr)};
    REQUIRE(raw);
    subclass_seen_id = 0;

    REQUIRE(winwrap::window::subclass(raw.get(), &recording_subclass, 9,
                                      reinterpret_cast<DWORD_PTR>(&subclass_seen_id)));
    winwrap::message::send(raw.get(), WM_APP + 7);
    CHECK(subclass_seen_id == 9);

    CHECK(winwrap::window::remove_subclass(raw.get(), &recording_subclass, 9));
    CHECK_FALSE(winwrap::window::remove_subclass(raw.get(), &recording_subclass, 9));  // gone

    subclass_seen_id = 0;
    winwrap::message::send(raw.get(), WM_APP + 7);
    CHECK(subclass_seen_id == 0);
}

TEST_CASE("timer operations reject a null window instead of using a thread timer") {
    using namespace std::chrono_literals;
    CHECK(winwrap::window::start_timer(nullptr, 1, 10ms).error().value() ==
          ERROR_INVALID_WINDOW_HANDLE);
    CHECK(winwrap::window::stop_timer(nullptr, 1).error().value() == ERROR_INVALID_WINDOW_HANDLE);
}

TEST_CASE("accept_files sets and clears a raw window's drop registration") {
    auto window = make_plain_window();
    const HWND raw = window->hwnd();

    winwrap::window::accept_files(raw);
    CHECK((*winwrap::window::ex_style(raw) & WS_EX_ACCEPTFILES) != 0);

    winwrap::window::accept_files(raw, false);
    CHECK((*winwrap::window::ex_style(raw) & WS_EX_ACCEPTFILES) == 0);
}

TEST_CASE("set_foreground reports that a null window cannot take the foreground") {
    CHECK_FALSE(winwrap::window::set_foreground(nullptr));
}
