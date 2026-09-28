#include "winwrap/disk_serial.hpp"

#include "winwrap/win.hpp"

#include <winioctl.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <vector>

#include "winwrap/error.hpp"

namespace winwrap {

std::expected<DiskSerial, std::error_code> DiskSerial::read(const Device& device,
                                                            const DiskSerialConfig& config) {
    constexpr auto minimum_size{offsetof(STORAGE_DEVICE_DESCRIPTOR, RawDeviceProperties) + 1};
    if (config.maximum_descriptor_size < minimum_size ||
        config.maximum_descriptor_size > std::numeric_limits<DWORD>::max())
        return std::unexpected{error::win32(ERROR_INVALID_PARAMETER)};

    STORAGE_PROPERTY_QUERY query{};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;
    const auto header{
        device.control<STORAGE_DESCRIPTOR_HEADER>(IOCTL_STORAGE_QUERY_PROPERTY, query)};
    if (!header)
        return std::unexpected{header.error().code};
    if (header->Size < minimum_size)
        return std::unexpected{error::win32(ERROR_INVALID_DATA)};
    if (header->Size > config.maximum_descriptor_size)
        return std::unexpected{error::win32(ERROR_INSUFFICIENT_BUFFER)};

    std::vector<std::byte> bytes(header->Size);
    const auto byte_count_result{
        device.control(IOCTL_STORAGE_QUERY_PROPERTY, std::as_bytes(std::span{&query, 1}), bytes)};
    if (!byte_count_result)
        return std::unexpected{byte_count_result.error().code};
    if (*byte_count_result != bytes.size())
        return std::unexpected{error::win32(ERROR_INVALID_DATA)};
    return decode(bytes);
}

std::expected<DiskSerial, std::error_code> DiskSerial::decode(std::span<const std::byte> bytes) {
    constexpr auto minimum_size{offsetof(STORAGE_DEVICE_DESCRIPTOR, RawDeviceProperties) + 1};
    if (bytes.size() < minimum_size)
        return std::unexpected{error::win32(ERROR_INVALID_DATA)};

    STORAGE_DEVICE_DESCRIPTOR descriptor{};
    std::memcpy(&descriptor, bytes.data(),
                offsetof(STORAGE_DEVICE_DESCRIPTOR, RawDeviceProperties));
    if (descriptor.Version < minimum_size || descriptor.Version > descriptor.Size ||
        descriptor.Size < minimum_size || descriptor.Size > bytes.size() ||
        descriptor.SerialNumberOffset < minimum_size ||
        descriptor.SerialNumberOffset >= descriptor.Size)
        return std::unexpected{error::win32(ERROR_INVALID_DATA)};

    const auto serial_bytes{bytes.subspan(descriptor.SerialNumberOffset,
                                          descriptor.Size - descriptor.SerialNumberOffset)};
    const auto end{std::ranges::find(serial_bytes, std::byte{})};
    if (end == serial_bytes.begin() || end == serial_bytes.end())
        return std::unexpected{error::win32(ERROR_INVALID_DATA)};
    std::string serial{};
    for (auto current{serial_bytes.begin()}; current != end; ++current) {
        const auto character{std::to_integer<unsigned char>(*current)};
        if (character < 0x20U || character > 0x7eU)
            return std::unexpected{error::win32(ERROR_INVALID_DATA)};
        serial += static_cast<char>(character);
    }
    return DiskSerial{std::move(serial)};
}

}
