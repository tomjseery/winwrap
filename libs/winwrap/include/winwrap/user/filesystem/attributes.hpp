#pragma once

#include "winwrap/user/mode.hpp"
#include "winwrap/user/win.hpp"

#include <expected>
#include <filesystem>
#include <system_error>

#include "winwrap/user/error.hpp"

/// Filesystem operations that std::filesystem does not cover.
namespace winwrap::user::filesystem {

/// The `FILE_ATTRIBUTE_*` bits currently set on `path`.
///
/// @return The bitmask, or the Win32 error. Callers never see
///         `GetFileAttributesW`'s failure sentinel, which is `INVALID_FILE_ATTRIBUTES`
///         (`0xFFFFFFFF`) rather than the usual zero.
[[nodiscard]] inline std::expected<DWORD, std::error_code> attributes(
    const std::filesystem::path& path) {
    const DWORD bits = GetFileAttributesW(path.c_str());
    if (bits == INVALID_FILE_ATTRIBUTES)
        return std::unexpected(error::last());
    return bits;
}

/// Replaces every attribute bit on `path` with `bits`.
///
/// @note Bits the OS owns (`FILE_ATTRIBUTE_DIRECTORY`, `_REPARSE_POINT`, ...) can't be
///       assigned and are ignored. Prefer add_attributes / remove_attributes
///       when changing individual bits.
[[nodiscard]] inline std::expected<void, std::error_code> set_attributes(
    const std::filesystem::path& path, DWORD bits) {
    return error::nonzero_or_last(SetFileAttributesW(path.c_str(), bits));
}

/// Turns `bits` on, keeping whatever bits `path` already has.
[[nodiscard]] inline std::expected<void, std::error_code> add_attributes(
    const std::filesystem::path& path, DWORD bits) {
    return attributes(path).and_then(
        [&](DWORD current) { return set_attributes(path, current | bits); });
}

/// Turns `bits` off, keeping whatever bits `path` already has.
[[nodiscard]] inline std::expected<void, std::error_code> remove_attributes(
    const std::filesystem::path& path, DWORD bits) {
    return attributes(path).and_then([&](DWORD current) {
        const DWORD remaining = current & ~bits;
        // Win32 spells "no attributes" as FILE_ATTRIBUTE_NORMAL, never as a zero mask.
        return set_attributes(path, remaining ? remaining : FILE_ATTRIBUTE_NORMAL);
    });
}

}  // namespace winwrap::user::filesystem
