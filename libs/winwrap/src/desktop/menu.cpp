#include "winwrap/user/desktop/menu.hpp"

#include "winwrap/user/desktop/message.hpp"
#include "winwrap/user/error.hpp"

namespace winwrap::user {

std::expected<Menu, std::error_code> Menu::create() {
    return error::nonzero_or_last(CreatePopupMenu()).transform([](HMENU h) {
        return Menu{wil::unique_hmenu{h}};
    });
}

std::expected<void, std::error_code> Menu::add_item(UINT id, const wchar_t* text) {
    return error::nonzero_or_last(AppendMenuW(handle_.get(), MF_STRING, id, text));
}

std::expected<void, std::error_code> Menu::add_item(const wchar_t* text,
                                                    std::function<void()> handler) {
    return error::nonzero_or_last(AppendMenuW(handle_.get(), MF_STRING, next_id_, text))
        .transform([&] { handlers_.emplace(next_id_++, std::move(handler)); });
}

void Menu::show(HWND owner) {
    SetForegroundWindow(owner);
    POINT pt{};
    GetCursorPos(&pt);
    const auto picked = static_cast<UINT>(
        TrackPopupMenuEx(handle_.get(), TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY, pt.x,
                         pt.y, owner, nullptr));
    // Best effort: the empty message only lets a background owner dismiss the menu.
    static_cast<void>(message::post(owner, WM_NULL));
    if (picked == 0)
        return;
    if (auto it = handlers_.find(picked); it != handlers_.end()) {
        // Copy first: the handler may do anything, including tearing down the
        // window that owns this menu -- the copy outlives us either way.
        auto handler = it->second;
        handler();
        return;
    }
    // An unreported failure here is existing debt M5 (menu posting failures).
    static_cast<void>(message::post(owner, WM_COMMAND, picked));
}

}  // namespace winwrap::user
