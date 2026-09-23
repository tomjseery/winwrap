#pragma once

#include <expected>
#include <span>
#include <string>
#include <system_error>
#include <vector>

namespace winwrap::detail {

[[nodiscard]] std::expected<std::vector<std::wstring>, std::error_code>
parse_device_interface_list(std::span<const wchar_t> characters);

}  // namespace winwrap::detail
