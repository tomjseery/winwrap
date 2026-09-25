#include <catch2/catch_test_macros.hpp>

#include <string>

#include "winwrap/desktop/window/native_window.hpp"

namespace {

std::wstring class_of(HWND window) {
    std::wstring name(256, L'\0');
    name.resize(static_cast<std::size_t>(GetClassNameW(window, name.data(), 256)));
    return name;
}

LRESULT CALLBACK rejecting_proc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_CREATE)
        return -1;  // refuse creation
    return DefWindowProcW(window, msg, wparam, lparam);
}

}  // namespace

TEST_CASE("create_window creates an owned window of a system class") {
    auto window = winwrap::create_window(
        {.class_name = L"STATIC", .title = L"native", .style = WS_OVERLAPPED, .x = -20000,
         .y = -20000, .width = 100, .height = 50});

    REQUIRE(window);
    const HWND raw = window->get();
    CHECK(class_of(raw) == L"Static");
    CHECK_FALSE(IsWindowVisible(raw));

    window->reset();  // the handle owns the window
    CHECK_FALSE(IsWindow(raw));
}

TEST_CASE("create_window gives a child window its id") {
    auto parent = winwrap::create_window({.class_name = L"STATIC", .parent = HWND_MESSAGE});
    REQUIRE(parent);

    auto child = winwrap::create_window({.class_name = L"BUTTON",
                                                .style = WS_CHILD,
                                                .x = 0,
                                                .y = 0,
                                                .width = 80,
                                                .height = 24,
                                                .parent = parent->get(),
                                                .child_id = 42});

    REQUIRE(child);
    CHECK(GetDlgCtrlID(child->get()) == 42);
    CHECK(GetParent(child->get()) == parent->get());
}

TEST_CASE("create_window reports an unregistered class") {
    auto window = winwrap::create_window({.class_name = L"WinwrapNoSuchClass"});

    REQUIRE_FALSE(window);
    CHECK(window.error().value() == ERROR_CANNOT_FIND_WND_CLASS);
}

TEST_CASE("create_window reports a window procedure that refuses creation") {
    WNDCLASSW wc{};
    wc.lpfnWndProc = rejecting_proc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"WinwrapRejectingClass";
    REQUIRE((RegisterClassW(&wc) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS));

    auto window = winwrap::create_window(
        {.class_name = L"WinwrapRejectingClass", .parent = HWND_MESSAGE});

    REQUIRE_FALSE(window);
    CHECK(window.error().value() == ERROR_INVALID_HANDLE);  // what Windows records
}
