#pragma once

#include "winwrap/win.hpp"

#include <expected>
#include <filesystem>
#include <system_error>

namespace winwrap {

/// The running executable's module handle (`GetModuleHandleW(nullptr)`). Never null and
/// valid for the life of the process.
[[nodiscard]] HMODULE current_module() noexcept;

/// An already-loaded module found by file name or path, e.g. `L"user32.dll"`
/// (`GetModuleHandleW(name)`).
/// @return A borrowed handle, or the Win32 error (`ERROR_MOD_NOT_FOUND` when no such module
///         is loaded). The handle is not reference-counted: it stays valid only while
///         something else keeps the module loaded.
[[nodiscard]] std::expected<HMODULE, std::error_code> loaded_module(
    const std::filesystem::path& name);

/// The full path of the file `module` was loaded from (`GetModuleFileNameW`, growing the
/// buffer until the path fits).
/// @return The path, or the Win32 error; `ERROR_INVALID_HANDLE` for a null `module`, which
///         `GetModuleFileNameW` would otherwise read as the executable.
[[nodiscard]] std::expected<std::filesystem::path, std::error_code> module_path(HMODULE module);

/// A borrowed view of a loaded module (an .exe or DLL). Holds the `HMODULE` without
/// affecting its load count, so copying a Module is free and destroying one frees nothing.
/// Its operations delegate to the free functions above, so raw-handle code can use either.
class Module final {
public:
    /// Views `handle`, which must be non-null and stay loaded while this Module is used.
    explicit Module(HMODULE handle) noexcept : handle_{handle} {}

    /// The running executable (current_module()).
    [[nodiscard]] static Module current() noexcept { return Module{current_module()}; }

    /// An already-loaded module by name or path (loaded_module()).
    [[nodiscard]] static std::expected<Module, std::error_code> loaded(
        const std::filesystem::path& name) {
        return loaded_module(name).transform([](HMODULE handle) { return Module{handle}; });
    }

    /// The file this module was loaded from (module_path()).
    [[nodiscard]] std::expected<std::filesystem::path, std::error_code> path() const {
        return module_path(handle_);
    }

    /// The borrowed native handle.
    [[nodiscard]] HMODULE handle() const noexcept { return handle_; }

    bool operator==(const Module&) const = default;

private:
    HMODULE handle_;
};

}  // namespace winwrap
