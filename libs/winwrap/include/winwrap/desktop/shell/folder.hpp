#pragma once

#include "winwrap/win.hpp"

#include <shlobj.h>

#include <filesystem>

namespace winwrap {

/// Notifies the Shell that `folder` changed so Explorer can refresh an open view,
/// for example after writing `desktop.ini`.
///
/// @note This call does not wait for Explorer to refresh its view.
inline void notify_folder_changed(const std::filesystem::path& folder) {
    SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATHW, folder.c_str(), nullptr);
}

}  // namespace winwrap
