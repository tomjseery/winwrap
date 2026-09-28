#pragma once

#include <type_traits>

namespace winwrap::protocol::device {

/// Data a device-control request carries as its own bytes: a trivially copyable struct,
/// array, number or enumeration. Pointers, handles and views such as std::span are
/// rejected, because their bytes are an address rather than the data they refer to.
template <typename T>
concept ControlData = std::is_trivially_copyable_v<T> &&
                      (std::is_aggregate_v<T> || std::is_arithmetic_v<T> || std::is_enum_v<T>);

}  // namespace winwrap::protocol::device
