#pragma once

#include "winwrap/detail/user_mode.hpp"

#include "winwrap/win.hpp"

#include <shlobj.h>

#include <filesystem>

/// Shell (Explorer) integration.
namespace winwrap::shell {

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
