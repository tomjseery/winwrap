#pragma once

#include "winwrap/detail/user_mode.hpp"

#include "winwrap/win.hpp"

#include <shlobj.h>
#include <wil/resource.h>

#include <array>
#include <expected>
#include <filesystem>
#include <optional>
#include <system_error>
#include <utility>

/// Shell (Explorer) integration.
namespace winwrap::shell {

/// One Shell change delivered to a ChangeRegistration's owner window: its event and its own
/// copies of the changed items' ID lists, freed on destruction (ILFree). The Shell's data
/// for a delivery is valid only while that delivery is processed, so lock() copies what it
/// needs and releases the Shell's lock before returning. Move-only; a moved-from
/// notification has no event and no items. The ShellChangeAware mixin creates one per
/// delivery; a window that routes messages itself calls lock().
class ChangeNotification final {
public:
    /// The message every ChangeRegistration posts to its owner, registered once per
    /// session under a Winwrap-specific name (RegisterWindowMessageW), so it cannot collide
    /// with an application's `WM_APP + n`.
    [[nodiscard]] static std::expected<UINT, std::error_code> message();

    /// Takes the change carried by one message() delivery: locks the Shell's data
    /// (SHChangeNotification_Lock), copies the item ID lists and unlocks
    /// (SHChangeNotification_Unlock). Call it while processing that delivery.
    /// @param wparam  The delivery's `wParam`, the Shell's handle to the change.
    /// @param lparam  The delivery's `lParam`, the id of the process that posted it.
    /// @return The change, or the error; `ERROR_GEN_FAILURE` when the Shell no longer
    ///         holds it and records no reason, `ERROR_NOT_ENOUGH_MEMORY` when an item
    ///         cannot be copied.
    [[nodiscard]] static std::expected<ChangeNotification, std::error_code> lock(WPARAM wparam,
                                                                                 LPARAM lparam);

    /// Takes over `other`'s event and items; `other` is left with neither.
    ChangeNotification(ChangeNotification&& other) noexcept
        : items_{std::move(other.items_)}, event_{std::exchange(other.event_, 0)} {}

    /// Frees this notification's items and takes over `other`'s event and items.
    ChangeNotification& operator=(ChangeNotification&& other) noexcept {
        items_ = std::move(other.items_);
        event_ = std::exchange(other.event_, 0);
        return *this;
    }

    ChangeNotification(const ChangeNotification&) = delete;
    ChangeNotification& operator=(const ChangeNotification&) = delete;
    ~ChangeNotification() = default;

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

    /// The two item ID lists, borrowed from this notification, which remains their owner.
    /// Either may be null; both are null for events that carry no items (see path()).
    [[nodiscard]] std::array<PCIDLIST_ABSOLUTE, 2> items() const noexcept {
        return {items_[0].get(), items_[1].get()};
    }

private:
    using unique_item_list = wil::unique_any<PIDLIST_ABSOLUTE, decltype(&::ILFree), ::ILFree>;

    explicit ChangeNotification(LONG event) noexcept : event_{event} {}

    std::array<unique_item_list, 2> items_;
    LONG event_{};
};

/// @name Telling the Shell what changed
/// Each function below tells the Shell about a change the application already made, so
/// open Explorer views and the icon cache can refresh (SHChangeNotify). None waits for
/// Explorer, and none reports failure: SHChangeNotify returns nothing. The file and folder
/// variants map to distinct Shell events; a deleted or renamed path can no longer be
/// inspected, so the caller says which kind it was. Each sends the long form of its paths:
/// the Shell matches items by their long names, so a notification naming an 8.3 short path
/// would reach no listener. A path that no longer exists keeps its own last component.
/// @{

/// A file was created at `file` (SHCNE_CREATE).
void notify_file_created(const std::filesystem::path& file);

/// A folder was created at `folder` (SHCNE_MKDIR).
void notify_folder_created(const std::filesystem::path& folder);

/// The file at `file` was deleted (SHCNE_DELETE).
void notify_file_deleted(const std::filesystem::path& file);

/// The folder at `folder` was deleted (SHCNE_RMDIR).
void notify_folder_deleted(const std::filesystem::path& folder);

/// A file was renamed or moved from `from` to `to` (SHCNE_RENAMEITEM).
void notify_file_renamed(const std::filesystem::path& from, const std::filesystem::path& to);

/// A folder was renamed or moved from `from` to `to` (SHCNE_RENAMEFOLDER).
void notify_folder_renamed(const std::filesystem::path& from, const std::filesystem::path& to);

/// The contents or attributes of the file at `file` changed (SHCNE_UPDATEITEM).
void notify_file_changed(const std::filesystem::path& file);

/// The contents of `folder` changed, for example after writing its `desktop.ini`
/// (SHCNE_UPDATEDIR).
void notify_folder_changed(const std::filesystem::path& folder);

/// File-type associations changed, so the Shell refreshes icons and handlers
/// (SHCNE_ASSOCCHANGED).
void notify_associations_changed();

/// @}

}  // namespace winwrap::shell
