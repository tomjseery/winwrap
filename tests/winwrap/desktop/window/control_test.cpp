#include "winwrap/desktop/window/control.hpp"

#include <catch2/catch_test_macros.hpp>

#include "winwrap/desktop/window/window.hpp"

namespace {
// A minimal derived control used only as a compile-time fixture: forming
// &TestControl::create (in the test below) forces the whole Control<T> chain to
// instantiate -- create -> create_control -> subclass_proc -> route_message --
// so the build compiles and links it. Two hooks are defined so both branches of
// the if-constexpr detection get checked. Exercising a control for real needs a
// live message pump; see CONTROL_PLAN.md section 8.
struct TestControl : winwrap::Control<TestControl> {
    static constexpr const wchar_t* class_name = L"BUTTON";
    void on_paint() {}
    void on_mouse_move(int /*x*/, int /*y*/) {}
};

struct ControlHost : winwrap::Window<ControlHost> {
    static constexpr const wchar_t* class_name = L"WinwrapDefaultProcControlHost";
};

struct RoutedControl : winwrap::Control<RoutedControl> {
    static constexpr const wchar_t* class_name = L"BUTTON";
    bool delegated_to_default{};

    LRESULT route_message(UINT msg, WPARAM wparam, LPARAM lparam) {
        delegated_to_default = true;
        return default_proc(msg, wparam, lparam);
    }
};

struct LeadingBase {
    void* padding{};
};

struct OffsetControl : LeadingBase, winwrap::Control<OffsetControl> {
    static constexpr const wchar_t* class_name = L"BUTTON";
    static constexpr UINT probe_message{WM_APP + 1};
    bool received{};

    LRESULT route_message(UINT msg, WPARAM wparam, LPARAM lparam) {
        if (msg == probe_message) {
            received = true;
            return 0;
        }
        return Control::route_message(msg, wparam, lparam);
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
    auto control = RoutedControl::create({.parent = host->hwnd(), .id = 1});
    REQUIRE(control);

    control->delegated_to_default = false;
    const LRESULT dialog_code = winwrap::message::send(control->hwnd(), WM_GETDLGCODE, 0, 0);

    REQUIRE(control->delegated_to_default);
    CHECK((dialog_code & DLGC_BUTTON) != 0);
}

TEST_CASE("Control factory carries a native creation error") {
    const auto control = RoutedControl::create({.id = 1});

    REQUIRE_FALSE(control);
    CHECK(control.error());
}

TEST_CASE("Control stores the adjusted final object pointer for subclass routing") {
    auto host = ControlHost::create({.parent = HWND_MESSAGE});
    REQUIRE(host);
    auto control = OffsetControl::create({.parent = host->hwnd(), .id = 1});
    REQUIRE(control);

    winwrap::message::send(control->hwnd(), OffsetControl::probe_message);

    CHECK(control->received);
}

TEST_CASE("Control owner destroys its child window when released first") {
    auto host = ControlHost::create({.parent = HWND_MESSAGE});
    REQUIRE(host);
    HWND child{};
    {
        auto control = RoutedControl::create({.parent = host->hwnd(), .id = 1});
        REQUIRE(control);
        child = control->hwnd();
        REQUIRE(IsWindow(child));
    }

    CHECK_FALSE(IsWindow(child));
}

TEST_CASE("Control owner detaches when its parent destroys the child first") {
    auto host = ControlHost::create({.parent = HWND_MESSAGE});
    REQUIRE(host);
    auto control = RoutedControl::create({.parent = host->hwnd(), .id = 1});
    REQUIRE(control);

    host->send(WM_CLOSE);

    CHECK(host->hwnd() == nullptr);
    CHECK(control->hwnd() == nullptr);
}
