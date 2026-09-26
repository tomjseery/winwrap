#include "winwrap/user/desktop/window/controls/edit.hpp"

#include <catch2/catch_test_macros.hpp>

#include "winwrap/user/desktop/window/window.hpp"

namespace {
struct EditHost : winwrap::user::Window<EditHost> {
    static constexpr const wchar_t* class_name = L"WinwrapEditHost";
};

constexpr UINT edit_id = 1;
}  // namespace

TEST_CASE("Edit fires on_text_changed on a reflected EN_CHANGE") {
    auto host = EditHost::create();
    REQUIRE(host);

    auto edit = winwrap::user::Edit::create({.parent = host->hwnd(), .id = edit_id});
    REQUIRE(edit);

    bool changed = false;
    edit->on_text_changed = [&] { changed = true; };

    winwrap::user::message::send(host->hwnd(), WM_COMMAND, MAKEWPARAM(edit_id, EN_CHANGE),
                           reinterpret_cast<LPARAM>(edit->hwnd()));

    REQUIRE(changed);
}

TEST_CASE("Edit contents round-trip through set_text / text") {
    auto host = EditHost::create();
    REQUIRE(host);

    auto edit = winwrap::user::Edit::create({.parent = host->hwnd(), .id = edit_id});
    REQUIRE(edit);

    REQUIRE(edit->text().empty());
    edit->set_text(L"hello");
    REQUIRE(edit->text() == L"hello");
}
