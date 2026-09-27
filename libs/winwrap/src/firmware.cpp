#include "winwrap/firmware.hpp"

#include <limits>

#include "winwrap/error.hpp"

namespace winwrap {

std::expected<FirmwareTable, std::error_code> FirmwareTable::read(const FirmwareQuery& query) {
    if (query.maximum_size == 0 || query.maximum_size > std::numeric_limits<UINT>::max())
        return std::unexpected{error::win32(ERROR_INVALID_PARAMETER)};

    auto size{::GetSystemFirmwareTable(query.provider, query.table_id, nullptr, 0)};
    if (size == 0)
        return std::unexpected{error::last()};

    constexpr unsigned retry_limit{3};
    for (unsigned attempt{}; attempt < retry_limit; ++attempt) {
        if (size > query.maximum_size)
            return std::unexpected{error::win32(ERROR_INSUFFICIENT_BUFFER)};
        std::vector<std::byte> bytes(size);
        const auto count{
            ::GetSystemFirmwareTable(query.provider, query.table_id, bytes.data(), size)};
        if (count == 0)
            return std::unexpected{error::last()};
        if (count <= size) {
            bytes.resize(count);
            return FirmwareTable{std::move(bytes)};
        }
        size = count;
    }
    return std::unexpected{error::win32(ERROR_RETRY)};
}

}  // namespace winwrap
