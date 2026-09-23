#include "winwrap/device.hpp"

#include <cfgmgr32.h>

#include <algorithm>
#include <limits>
#include <utility>

#include "device_interface_list.hpp"
#include "winwrap/error.hpp"

namespace winwrap {
namespace {

[[nodiscard]] std::error_code system_error(DWORD code) {
    return {static_cast<int>(code), std::system_category()};
}

[[nodiscard]] std::error_code configuration_error(CONFIGRET result) {
    return system_error(::CM_MapCrToWin32Err(result, ERROR_GEN_FAILURE));
}

}  // namespace

namespace detail {

std::expected<std::vector<std::wstring>, std::error_code>
parse_device_interface_list(std::span<const wchar_t> characters) {
    if (characters.empty())
        return std::unexpected(system_error(ERROR_INVALID_DATA));

    std::vector<std::wstring> paths;
    std::size_t position{};
    while (position < characters.size()) {
        const auto end{std::find(characters.begin() + position, characters.end(), L'\0')};
        if (end == characters.end())
            return std::unexpected(system_error(ERROR_INVALID_DATA));

        const auto length{static_cast<std::size_t>(end - (characters.begin() + position))};
        if (length == 0) {
            if (std::all_of(end, characters.end(), [](wchar_t c) { return c == L'\0'; }))
                return paths;
            return std::unexpected(system_error(ERROR_INVALID_DATA));
        }

        paths.emplace_back(characters.data() + position, length);
        position += length + 1;
    }

    return std::unexpected(system_error(ERROR_INVALID_DATA));
}

}  // namespace detail

std::expected<std::vector<std::wstring>, std::error_code> Device::paths(const GUID& interface_id) {
    constexpr unsigned int retry_limit{3};
    auto class_id{interface_id};
    for (unsigned int attempt{}; attempt < retry_limit; ++attempt) {
        ULONG characters{};
        const auto sizing{::CM_Get_Device_Interface_List_SizeW(
            &characters, &class_id, nullptr, CM_GET_DEVICE_INTERFACE_LIST_PRESENT)};
        if (sizing != CR_SUCCESS)
            return std::unexpected(configuration_error(sizing));
        if (characters == 0)
            return std::unexpected(system_error(ERROR_INVALID_DATA));

        std::vector<wchar_t> list(characters, L'\0');
        const auto listing{::CM_Get_Device_Interface_ListW(
            &class_id, nullptr, list.data(), characters, CM_GET_DEVICE_INTERFACE_LIST_PRESENT)};
        if (listing == CR_BUFFER_SMALL)
            continue;
        if (listing != CR_SUCCESS)
            return std::unexpected(configuration_error(listing));
        return detail::parse_device_interface_list(list);
    }

    return std::unexpected(system_error(ERROR_RETRY));
}

std::expected<Device, std::error_code> Device::open(const Config& config) {
    const HANDLE raw{::CreateFileW(config.path.c_str(), config.access, config.share_mode,
                                   nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)};
    if (raw == INVALID_HANDLE_VALUE)
        return std::unexpected(last_error());
    return Device{wil::unique_hfile{raw}};
}

std::expected<std::size_t, std::error_code>
Device::control(DWORD code, std::span<const std::byte> input,
                std::span<std::byte> output) const {
    constexpr auto maximum{static_cast<std::size_t>(std::numeric_limits<DWORD>::max())};
    if (input.size() > maximum || output.size() > maximum)
        return std::unexpected(system_error(ERROR_INVALID_PARAMETER));

    DWORD returned{};
    const BOOL succeeded{::DeviceIoControl(
        handle_.get(), code,
        input.empty() ? nullptr : const_cast<std::byte*>(input.data()),
        static_cast<DWORD>(input.size()),
        output.empty() ? nullptr : output.data(),
        static_cast<DWORD>(output.size()), &returned, nullptr)};
    if (succeeded == FALSE)
        return std::unexpected(last_error());
    if (returned > output.size())
        return std::unexpected(system_error(ERROR_INVALID_DATA));
    return static_cast<std::size_t>(returned);
}

}  // namespace winwrap
