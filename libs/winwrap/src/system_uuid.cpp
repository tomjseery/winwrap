#include "winwrap/system_uuid.hpp"

#include "winwrap/win.hpp"

#include <algorithm>
#include <array>
#include <cstdint>

#include "winwrap/error.hpp"

namespace winwrap {
namespace {

std::unexpected<std::error_code> invalid_data() {
    return std::unexpected{error::win32(ERROR_INVALID_DATA)};
}

std::uint32_t little_endian_size(std::span<const std::byte> bytes) {
    std::uint32_t value{};
    for (std::size_t index{}; index < sizeof(value); ++index)
        value |= static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[index]))
                 << (index * 8U);
    return value;
}

std::string format_uuid(std::span<const std::byte, 16> bytes) {
    constexpr std::array<std::size_t, 16> order{3, 2, 1,  0,  5,  4,  7,  6,
                                                8, 9, 10, 11, 12, 13, 14, 15};
    constexpr std::string_view digits{"0123456789abcdef"};
    std::string value{};
    value.reserve(36);
    for (std::size_t index{}; index < order.size(); ++index) {
        if (index == 4 || index == 6 || index == 8 || index == 10)
            value += '-';
        const auto octet{std::to_integer<unsigned char>(bytes[order[index]])};
        value += digits[octet >> 4U];
        value += digits[octet & 0x0fU];
    }
    return value;
}

}  // namespace

std::expected<SystemUuid, std::error_code> SystemUuid::decode(std::span<const std::byte> bytes) {
    constexpr std::size_t raw_header_size{8};
    constexpr std::size_t uuid_offset{8};
    constexpr std::size_t uuid_size{16};
    constexpr std::size_t type1_size{uuid_offset + uuid_size};
    if (bytes.size() < raw_header_size || (std::to_integer<unsigned char>(bytes[1]) < 2 ||
                                           (std::to_integer<unsigned char>(bytes[1]) == 2 &&
                                            std::to_integer<unsigned char>(bytes[2]) < 6)))
        return invalid_data();

    const auto table_size{little_endian_size(bytes.subspan(4, 4))};
    if (table_size != bytes.size() - raw_header_size)
        return invalid_data();

    const auto table{bytes.subspan(raw_header_size)};
    for (std::size_t offset{}; offset < table.size();) {
        if (table.size() - offset < 4)
            return invalid_data();
        const auto type{std::to_integer<unsigned char>(table[offset])};
        const auto length{std::to_integer<unsigned char>(table[offset + 1])};
        if (length < 4 || length > table.size() - offset)
            return invalid_data();

        auto next{offset + length};
        while (next + 1 < table.size() &&
               !(table[next] == std::byte{} && table[next + 1] == std::byte{}))
            ++next;
        if (next + 1 >= table.size())
            return invalid_data();

        if (type == 127)
            break;

        if (type == 1) {
            if (length < type1_size)
                return invalid_data();
            const auto uuid{std::span<const std::byte, uuid_size>{
                table.subspan(offset + uuid_offset, uuid_size)}};
            if (std::ranges::all_of(uuid, [](std::byte byte) { return byte == std::byte{}; }) ||
                std::ranges::all_of(uuid, [](std::byte byte) { return byte == std::byte{0xff}; }))
                return invalid_data();
            return SystemUuid{format_uuid(uuid)};
        }
        offset = next + 2;
    }
    return invalid_data();
}

std::expected<SystemUuid, std::error_code> SystemUuid::read(const FirmwareTableSource& firmware) {
    const auto bytes_result{firmware.read()};
    if (!bytes_result)
        return std::unexpected{bytes_result.error()};
    return decode(bytes_result->bytes());
}

}
