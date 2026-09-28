#pragma once

#include "winwrap/detail/user_mode.hpp"

#include "winwrap/win.hpp"

#include <shlobj.h>
#include <wil/resource.h>

#include <expected>
#include <filesystem>
#include <optional>
#include <span>
#include <system_error>
#include <utility>

/// Shell (Explorer) integration.
namespace winwrap::shell {

/// One Shell change delivered to a ChangeRegistration's owner window. Holds the Shell's
/// shared memory for the change (SHChangeNotification_Lock) and releases it on
/// destruction (SHChangeNotification_Unlock). Move-only. The ShellChangeAware mixin
/// creates one per delivery; a window that routes messages itself calls lock().
class ChangeNotification final {
public:
    /// The message every ChangeRegistration posts to its owner, registered once per
    /// session under a Winwrap-specific name (RegisterWindowMessageW), so it cannot collide
    /// with an application's `WM_APP + n`.
    [[nodiscard]] static std::expected<UINT, std::error_code> message();

    /// Takes the change carried by one message() delivery.
    /// @param wparam  The delivery's `wParam`, the Shell's handle to the change.
    /// @param lparam  The delivery's `lParam`, the id of the process that posted it.
    /// @return The change, or the error; `ERROR_GEN_FAILURE` when the Shell no longer
    ///         holds it and records no reason.
    [[nodiscard]] static std::expected<ChangeNotification, std::error_code> lock(WPARAM wparam,
                                                                                 LPARAM lparam);

    /// The SHCNE_* event. `SHCNE_INTERRUPT` is also set when the file system, rather than a
    /// Shell notification, reported it.
    [[nodiscard]] LONG event() const noexcept { return event_; }

    /// The changed item's file-system path (the previous path for a rename). Empty when the
    /// item is outside the file system, such as a printer, or the event carries no item,
    /// such as `SHCNE_ASSOCCHANGED`.
    [[nodiscard]] std::optional<std::filesystem::path> path() const;

    /// A renamed item's new file-system path; empty for every event other than
    /// `SHCNE_RENAMEITEM` and `SHCNE_RENAMEFOLDER`.
    [[nodiscard]] std::optional<std::filesystem::path> new_path() const;

    /// The event's two native item ID lists, either of which may be null. Borrowed: valid
    /// only while this ChangeNotification lives, and meaningful only for events that carry
    /// items (see path()).
    [[nodiscard]] std::span<const PIDLIST_ABSOLUTE, 2> items() const noexcept {
        return std::span<const PIDLIST_ABSOLUTE, 2>{items_, 2};
    }

private:
    using unique_lock = wil::unique_any<HANDLE, decltype(&::SHChangeNotification_Unlock),
                                        ::SHChangeNotification_Unlock>;

    ChangeNotification(unique_lock lock, PIDLIST_ABSOLUTE* items, LONG event) noexcept
        : lock_{std::move(lock)}, items_{items}, event_{event} {}

    unique_lock lock_;
    PIDLIST_ABSOLUTE* items_{};
    LONG event_{};
};

// Every function below tells the Shell about a change the application already made, so
// open Explorer views and the icon cache can refresh (SHChangeNotify). None waits for
// Explorer, and none reports failure: SHChangeNotify returns nothing. The file and folder
// variants map to distinct Shell events; a deleted or renamed path can no longer be
// inspected, so the caller says which kind it was.

/// A file was created at `file` (SHCNE_CREATE).
inline void notify_file_created(const std::filesystem::path& file) {
    SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW, file.c_str(), nullptr);
}

/// A folder was created at `folder` (SHCNE_MKDIR).
inline void notify_folder_created(const std::filesystem::path& folder) {
    SHChangeNotify(SHCNE_MKDIR, SHCNF_PATHW, folder.c_str(), nullptr);
}

/// The file at `file` was deleted (SHCNE_DELETE).
inline void notify_file_deleted(const std::filesystem::path& file) {
    SHChangeNotify(SHCNE_DELETE, SHCNF_PATHW, file.c_str(), nullptr);
}

/// The folder at `folder` was deleted (SHCNE_RMDIR).
inline void notify_folder_deleted(const std::filesystem::path& folder) {
    SHChangeNotify(SHCNE_RMDIR, SHCNF_PATHW, folder.c_str(), nullptr);
}

/// A file was renamed or moved from `from` to `to` (SHCNE_RENAMEITEM).
inline void notify_file_renamed(const std::filesystem::path& from,
                                const std::filesystem::path& to) {
    SHChangeNotify(SHCNE_RENAMEITEM, SHCNF_PATHW, from.c_str(), to.c_str());
}

/// A folder was renamed or moved from `from` to `to` (SHCNE_RENAMEFOLDER).
inline void notify_folder_renamed(const std::filesystem::path& from,
                                  const std::filesystem::path& to) {
    SHChangeNotify(SHCNE_RENAMEFOLDER, SHCNF_PATHW, from.c_str(), to.c_str());
}

/// The contents or attributes of the file at `file` changed (SHCNE_UPDATEITEM).
inline void notify_file_changed(const std::filesystem::path& file) {
    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATHW, file.c_str(), nullptr);
}

/// The contents of `folder` changed, for example after writing its `desktop.ini`
/// (SHCNE_UPDATEDIR).
inline void notify_folder_changed(const std::filesystem::path& folder) {
    SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATHW, folder.c_str(), nullptr);
}

/// File-type associations changed, so the Shell refreshes icons and handlers
/// (SHCNE_ASSOCCHANGED).
inline void notify_associations_changed() {
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
}

}  // namespace winwrap::shell
