#pragma once

#include "winwrap/detail/user_mode.hpp"

#include "winwrap/win.hpp"

#include <cstddef>
#include <expected>
#include <span>
#include <system_error>
#include <utility>
#include <vector>

namespace winwrap {

struct FirmwareQuery {
    DWORD provider;
    DWORD table_id;
    std::size_t maximum_size;
};

class FirmwareTable final {
public:
    explicit FirmwareTable(std::vector<std::byte> bytes) noexcept : bytes_{std::move(bytes)} {}

    [[nodiscard]] static std::expected<FirmwareTable, std::error_code> read(
        const FirmwareQuery& query);

    [[nodiscard]] std::span<const std::byte> bytes() const& noexcept { return bytes_; }
    std::span<const std::byte> bytes() const&& = delete;

private:
    std::vector<std::byte> bytes_;
};

}
