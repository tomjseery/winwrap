#include "winwrap/desktop/shell/change_notification.hpp"

#include <cstddef>
#include <cwchar>
#include <span>
#include <string>

#include "winwrap/desktop/message.hpp"
#include "winwrap/error.hpp"

namespace winwrap::shell {
namespace {

using unique_lock = wil::unique_any<HANDLE, decltype(&::SHChangeNotification_Unlock),
                                    ::SHChangeNotification_Unlock>;

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

[[nodiscard]] std::filesystem::path long_path(const std::filesystem::path& path) {
    if (const DWORD required{::GetLongPathNameW(path.c_str(), nullptr, 0)}; required != 0) {
        std::wstring long_form(required, L'\0');
        const DWORD written{::GetLongPathNameW(path.c_str(), long_form.data(),
                                               static_cast<DWORD>(long_form.size()))};
        if (written != 0 && written < long_form.size()) {
            long_form.resize(written);
            return std::filesystem::path{std::move(long_form)};
        }
    }
    const auto parent{path.parent_path()};
    if (parent.empty() || parent == path)
        return path;
    return long_path(parent) / path.filename();
}

void notify(LONG event, const std::filesystem::path& item) {
    ::SHChangeNotify(event, SHCNF_PATHW, long_path(item).c_str(), nullptr);
}

void notify(LONG event, const std::filesystem::path& from, const std::filesystem::path& to) {
    ::SHChangeNotify(event, SHCNF_PATHW, long_path(from).c_str(), long_path(to).c_str());
}

}  // namespace

void notify_file_created(const std::filesystem::path& file) {
    notify(SHCNE_CREATE, file);
}

void notify_folder_created(const std::filesystem::path& folder) {
    notify(SHCNE_MKDIR, folder);
}

void notify_file_deleted(const std::filesystem::path& file) {
    notify(SHCNE_DELETE, file);
}

void notify_folder_deleted(const std::filesystem::path& folder) {
    notify(SHCNE_RMDIR, folder);
}

void notify_file_renamed(const std::filesystem::path& from, const std::filesystem::path& to) {
    notify(SHCNE_RENAMEITEM, from, to);
}

void notify_folder_renamed(const std::filesystem::path& from, const std::filesystem::path& to) {
    notify(SHCNE_RENAMEFOLDER, from, to);
}

void notify_file_changed(const std::filesystem::path& file) {
    notify(SHCNE_UPDATEITEM, file);
}

void notify_folder_changed(const std::filesystem::path& folder) {
    notify(SHCNE_UPDATEDIR, folder);
}

void notify_associations_changed() {
    ::SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
}

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
            const unique_lock held{locked};
            ChangeNotification change{event};
            if (!items || !carries_items(event))
                return change;
            const std::span<const PIDLIST_ABSOLUTE, 2> locked_items{items, 2};
            for (std::size_t index{}; index < change.items_.size(); ++index) {
                if (!locked_items[index])
                    continue;
                change.items_[index].reset(::ILCloneFull(locked_items[index]));
                if (!change.items_[index])
                    return std::unexpected(error::win32(ERROR_NOT_ENOUGH_MEMORY));
            }
            return change;
        });
}

std::optional<std::filesystem::path> ChangeNotification::path() const {
    return file_system_path(items_[0].get());
}

std::optional<std::filesystem::path> ChangeNotification::new_path() const {
    if ((event_ & (SHCNE_RENAMEITEM | SHCNE_RENAMEFOLDER)) == 0)
        return std::nullopt;
    return file_system_path(items_[1].get());
}

}  // namespace winwrap::shell
