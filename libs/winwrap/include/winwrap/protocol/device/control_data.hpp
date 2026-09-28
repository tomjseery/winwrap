#pragma once

#include <concepts>
#include <type_traits>

namespace winwrap::protocol::device {

/// The input or output data of a Windows I/O control request, copied as its bytes: a
/// trivially copyable struct, array, number or enumeration that can be default-initialized
/// to receive into. Pointers, handles and views such as std::span are rejected, because
/// their bytes are an address rather than the data.
template <typename T>
concept ControlData = std::is_trivially_copyable_v<T> && std::default_initializable<T> &&
                      (std::is_aggregate_v<T> || std::is_arithmetic_v<T> || std::is_enum_v<T>);

}  // namespace winwrap::protocol::device
