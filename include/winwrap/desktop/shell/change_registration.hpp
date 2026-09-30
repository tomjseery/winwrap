#pragma once

#include "winwrap/detail/user_mode.hpp"

#include "winwrap/win.hpp"

#include <shlobj.h>
#include <wil/resource.h>

#include <expected>
#include <filesystem>
#include <system_error>
#include <utility>

namespace winwrap::shell {

/// Settings passed to ChangeRegistration::create; omitted fields take the defaults.
struct ChangeRegistrationConfig {
    HWND owner{};                  ///< Window that receives each change.
    std::filesystem::path folder;  ///< Folder whose changes are reported; must exist.
    bool recursive{};              ///< Also report changes anywhere below `folder`.
    LONG events{SHCNE_ALLEVENTS};  ///< The SHCNE_* events to report.
    /// `SHCNRF_ShellLevel` (changes reported through the Shell, such as `notify_*`) and/or
    /// `SHCNRF_InterruptLevel` (changes the file system reports). The registration adds
    /// the delivery and recursion flags itself.
    int sources{SHCNRF_ShellLevel | SHCNRF_InterruptLevel};
};

/// Reports Shell changes in one folder to a window (SHChangeNotifyRegister). While it
/// lives, each change is posted to the owner as ChangeNotification::message(), which the
/// ShellChangeAware mixin routes to `on_shell_change`; destroying it unregisters
/// (SHChangeNotifyDeregister). Move-only, so exactly one object owns each registration.
/// The owner window must pump messages on the thread that created it.
class ChangeRegistration final {
public:
    /// Registers `config.folder` for `config.owner`.
    /// @return The registration, or the error: `ERROR_INVALID_WINDOW_HANDLE` for a null
    ///         owner, the parse error for a folder the Shell cannot resolve (such as
    ///         `ERROR_FILE_NOT_FOUND`), and `ERROR_GEN_FAILURE` when Windows refuses the
    ///         registration without recording why.
    [[nodiscard]] static std::expected<ChangeRegistration, std::error_code> create(
        const ChangeRegistrationConfig& config);

    /// The native registration id, or 0 for a moved-from registration. This
    /// ChangeRegistration remains its owner.
    [[nodiscard]] ULONG id() const noexcept { return id_.get(); }

private:
    using unique_registration =
        wil::unique_any<ULONG, decltype(&::SHChangeNotifyDeregister), ::SHChangeNotifyDeregister,
                        wil::details::pointer_access_all, ULONG, ULONG, 0, ULONG>;

    explicit ChangeRegistration(unique_registration id) noexcept : id_{std::move(id)} {}

    unique_registration id_;
};

}  // namespace winwrap::shell
