#pragma once

#ifndef _KERNEL_MODE
#error WinWrap kernel memory operations require kernel mode
#endif

#include <ntddk.h>

#include <type_traits>

namespace winwrap::kernel::memory {

/// A type whose objects can be copied or zeroed byte by byte.
template <typename T>
concept TriviallyCopyable = std::is_trivially_copyable_v<T>;

/// Set every byte of an object, including padding, to zero (RtlZeroMemory).
/// Zero a record before filling it for user mode so its padding cannot disclose kernel memory.
void zero(TriviallyCopyable auto& object) noexcept {
    RtlZeroMemory(&object, sizeof(object));
}

/// Copy an object's bytes onto another object of the same type (RtlCopyMemory).
/// Both sides share one type, so their sizes always match; arrays are copied whole.
/// @pre The objects do not overlap.
template <TriviallyCopyable T>
void copy(T& destination, const T& source) noexcept {
    RtlCopyMemory(&destination, &source, sizeof(T));
}

}  // namespace winwrap::kernel::memory
