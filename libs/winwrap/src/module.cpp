#include "winwrap/module.hpp"

#include <algorithm>
#include <string>

#include "winwrap/error.hpp"

namespace winwrap {

HMODULE current_module() noexcept {
    return GetModuleHandleW(nullptr);
}

std::expected<HMODULE, std::error_code> loaded_module(const std::filesystem::path& name) {
    return check(GetModuleHandleW(name.c_str()));
}

std::expected<std::filesystem::path, std::error_code> module_path(HMODULE module) {
    if (!module)
        return std::unexpected(win32_error(ERROR_INVALID_HANDLE));

    // The documented limit for an extended-length Windows path.
    constexpr DWORD maximum_characters{32768};
    std::wstring buffer(MAX_PATH, L'\0');
    while (true) {
        const DWORD capacity = static_cast<DWORD>(buffer.size());
        const DWORD length = GetModuleFileNameW(module, buffer.data(), capacity);
        if (length == 0)
            return std::unexpected(last_error());
        // A full buffer means the path was truncated (ERROR_INSUFFICIENT_BUFFER).
        if (length < capacity) {
            buffer.resize(length);
            return std::filesystem::path{std::move(buffer)};
        }
        if (capacity >= maximum_characters)
            return std::unexpected(
                win32_error(ERROR_INSUFFICIENT_BUFFER));
        buffer.resize(std::min<std::size_t>(buffer.size() * 2, maximum_characters));
    }
}

}  // namespace winwrap
