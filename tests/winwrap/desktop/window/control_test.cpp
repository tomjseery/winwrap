#include <catch2/catch_test_macros.hpp>

#include "winwrap/desktop/window/control.hpp"
#include "winwrap/desktop/window/window.hpp"

namespace {
// A minimal derived control used only as a compile-time fixture: forming
// &TestControl::create (in the test below) forces the whole Control<T> chain to
// instantiate -- create -> create_control -> subclass_proc -> route_message --
// so the build compiles and links it. Two hooks are defined so both branches of
// the if-constexpr detection get checked. Exercising a control for real needs a
// live message pump; see CONTROL_PLAN.md section 8.
struct TestControl : winwrap::Control<TestControl> {
    static constexpr const wchar_t* control_class = L"BUTTON";
    void on_paint() {}
    void on_mouse_move(int /*x*/, int /*y*/) {}
};

struct ControlHost : winwrap::Window<ControlHost> {
    static constexpr const wchar_t* window_class_name = L"WinwrapDefaultProcControlHost";
};

struct RoutedControl : winwrap::Control<RoutedControl> {
    static constexpr const wchar_t* control_class = L"BUTTON";
    bool delegated_to_default{};

    LRESULT route_message(UINT msg, WPARAM wparam, LPARAM lparam) {
        delegated_to_default = true;
        return default_proc(msg, wparam, lparam);
    }
};
}  // namespace

TEST_CASE("Control<T> compiles and links") {
    (void)&TestControl::create;  // forms the address -> forces the template chain to compile
    REQUIRE(true);
}

TEST_CASE("Control custom routing can delegate to its subclass default procedure") {
    auto host = ControlHost::create({.parent = HWND_MESSAGE});
    REQUIRE(host);
    auto control = RoutedControl::create({.parent = (*host)->hwnd(), .id = 1});
    REQUIRE(control);

    (*control)->delegated_to_default = false;
    const LRESULT dialog_code = winwrap::message::send((*control)->hwnd(), WM_GETDLGCODE, 0, 0);

    REQUIRE((*control)->delegated_to_default);
    CHECK((dialog_code & DLGC_BUTTON) != 0);
}
