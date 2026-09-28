#include "winwrap/desktop/shell/change_notification.hpp"

#include <cwchar>
#include <string>

#include "winwrap/desktop/message.hpp"
#include "winwrap/error.hpp"

namespace winwrap::shell {
namespace {

// The events whose two "items" are not item ID lists, per Microsoft's ChangeNotifyWatcher
// sample: SHCNE_UPDATEIMAGE, for example, carries a system image list index.
[[nodiscard]] bool carries_items(LONG event) noexcept {
    constexpr LONG without_items{SHCNE_UPDATEIMAGE | SHCNE_ASSOCCHANGED | SHCNE_EXTENDED_EVENT |
                                 SHCNE_FREESPACE | SHCNE_DRIVEADDGUI | SHCNE_SERVERDISCONNECT};
    return (event & without_items) == 0;
}

[[nodiscard]] std::optional<std::filesystem::path> file_system_path(PCIDLIST_ABSOLUTE item) {
    if (!item)
        return std::nullopt;
    std::wstring path(UNICODE_STRING_MAX_CHARS, L'\0');
    if (!::SHGetPathFromIDListEx(item, path.data(), static_cast<DWORD>(path.size()),
                                 GPFIDL_DEFAULT))
        return std::nullopt;
    path.resize(std::wcslen(path.c_str()));
    return std::filesystem::path{std::move(path)};
}

}  // namespace

std::expected<UINT, std::error_code> ChangeNotification::message() {
    static const auto registered{winwrap::message::register_(L"WinwrapShellChangeNotification")};
    return registered;
}

std::expected<ChangeNotification, std::error_code> ChangeNotification::lock(WPARAM wparam,
                                                                            LPARAM lparam) {
    PIDLIST_ABSOLUTE* items{};
    LONG event{};
    return error::result_or_last([&] {
               return ::SHChangeNotification_Lock(reinterpret_cast<HANDLE>(wparam),
                                                  static_cast<DWORD>(lparam), &items, &event);
           })
        .and_then([&](HANDLE locked) -> std::expected<ChangeNotification, std::error_code> {
            if (!locked)
                return std::unexpected(error::win32(ERROR_GEN_FAILURE));
            return ChangeNotification{unique_lock{locked}, items, event};
        });
}

std::optional<std::filesystem::path> ChangeNotification::path() const {
    if (!carries_items(event_))
        return std::nullopt;
    return file_system_path(items()[0]);
}

std::optional<std::filesystem::path> ChangeNotification::new_path() const {
    if ((event_ & (SHCNE_RENAMEITEM | SHCNE_RENAMEFOLDER)) == 0)
        return std::nullopt;
    return file_system_path(items()[1]);
}

}  // namespace winwrap::shell
