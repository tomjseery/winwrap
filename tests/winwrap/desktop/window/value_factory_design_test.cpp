#include "winwrap/win.hpp"

#include <commctrl.h>

#include <catch2/catch_test_macros.hpp>
#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <utility>

#include "winwrap/desktop/window/control.hpp"
#include "winwrap/desktop/window/native_window.hpp"
#include "winwrap/module.hpp"

namespace {
constexpr UINT probe_message{WM_APP + 41};
constexpr UINT_PTR probe_subclass_id{41};

void* observed_subclass_receiver{};
void* observed_window_receiver{};

LRESULT CALLBACK probe_subclass_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR,
                                     DWORD_PTR ref_data) {
    if (msg == probe_message)
        observed_subclass_receiver = reinterpret_cast<void*>(ref_data);
    return DefSubclassProc(hwnd, msg, wparam, lparam);
}

LRESULT CALLBACK probe_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lparam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }
    if (msg == probe_message)
        observed_window_receiver = reinterpret_cast<void*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

struct ArrowTarget {
    void show() {}
};

struct ArrowOwner {
    ArrowTarget* operator->() { return &target; }

    ArrowTarget target;
};

template <typename T>
concept CanArrowToShow = requires(T value) { value->show(); };

static_assert(CanArrowToShow<ArrowOwner>);
static_assert(!CanArrowToShow<std::expected<ArrowOwner, int>>);

struct CaptureProbe {
    explicit CaptureProbe(const CaptureProbe** observed) : observed{observed} {
        callback = [this] { *this->observed = this; };
    }

    const CaptureProbe** observed;
    std::function<void()> callback;
};

struct DispatchProbe {
    std::unique_ptr<int> state{std::make_unique<int>(1)};

    void move_during_handler(std::optional<DispatchProbe>& destination, bool& handler_kept_state) {
        destination.emplace(std::move(*this));
        handler_kept_state = state != nullptr;
    }
};

struct LeadingBase {
    void* padding{};
};

struct OffsetControl : LeadingBase, winwrap::Control<OffsetControl> {
    static constexpr const wchar_t* control_class{L"BUTTON"};
};
}  // namespace

TEST_CASE("SetWindowSubclass can replace its stored object address") {
    auto control = winwrap::window::create(
        {.class_name = L"BUTTON", .style = WS_CHILD, .parent = HWND_MESSAGE});
    REQUIRE(control);

    int first{};
    int second{};
    REQUIRE(SetWindowSubclass(control->get(), probe_subclass_proc, probe_subclass_id,
                              reinterpret_cast<DWORD_PTR>(&first)));

    observed_subclass_receiver = nullptr;
    SendMessageW(control->get(), probe_message, 0, 0);
    CHECK(observed_subclass_receiver == &first);

    REQUIRE(SetWindowSubclass(control->get(), probe_subclass_proc, probe_subclass_id,
                              reinterpret_cast<DWORD_PTR>(&second)));
    observed_subclass_receiver = nullptr;
    SendMessageW(control->get(), probe_message, 0, 0);
    CHECK(observed_subclass_receiver == &second);

    REQUIRE(RemoveWindowSubclass(control->get(), probe_subclass_proc, probe_subclass_id));
}

TEST_CASE("GWLP_USERDATA can replace its stored object address") {
    constexpr const wchar_t* class_name{L"WinwrapValueFactoryDesignProbe"};
    WNDCLASSW wc{};
    wc.lpfnWndProc = probe_window_proc;
    wc.hInstance = winwrap::module::current();
    wc.lpszClassName = class_name;
    REQUIRE((RegisterClassW(&wc) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS));

    int first{};
    int second{};
    auto window = winwrap::window::create(
        {.class_name = class_name, .parent = HWND_MESSAGE, .create_param = &first});
    REQUIRE(window);

    observed_window_receiver = nullptr;
    SendMessageW(window->get(), probe_message, 0, 0);
    CHECK(observed_window_receiver == &first);

    SetLastError(ERROR_SUCCESS);
    const auto previous =
        SetWindowLongPtrW(window->get(), GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&second));
    REQUIRE(reinterpret_cast<void*>(previous) == &first);
    REQUIRE(GetLastError() == ERROR_SUCCESS);

    observed_window_receiver = nullptr;
    SendMessageW(window->get(), probe_message, 0, 0);
    CHECK(observed_window_receiver == &second);
}

TEST_CASE("moving a callback owner does not retarget a this capture") {
    const CaptureProbe* observed{};
    CaptureProbe source{&observed};
    CaptureProbe destination{std::move(source)};

    destination.callback();

    CHECK(observed == &source);
    CHECK(observed != &destination);
}

TEST_CASE("a handler that moves its receiver continues on the moved-from object") {
    DispatchProbe source;
    std::optional<DispatchProbe> destination;
    bool handler_kept_state{true};

    source.move_during_handler(destination, handler_kept_state);

    REQUIRE(destination);
    CHECK(destination->state != nullptr);
    CHECK_FALSE(handler_kept_state);
}

TEST_CASE("a Control base pointer needs adjustment to recover its final type") {
    OffsetControl control;
    auto* base = static_cast<winwrap::Control<OffsetControl>*>(&control);

    CHECK(static_cast<OffsetControl*>(base) == &control);
    CHECK(reinterpret_cast<OffsetControl*>(base) != &control);
}
