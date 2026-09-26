#pragma once

#include "winwrap/kernel/driver/detail/wdf.hpp"

namespace winwrap::kernel::driver {

/// A kernel operation's value together with its NTSTATUS result.
template <typename T>
class Result final {
public:
    /// Construct a successful result.
    [[nodiscard]] static constexpr Result success(T value) noexcept {
        return Result{STATUS_SUCCESS, value};
    }

    /// Construct a failed result with an inert value.
    [[nodiscard]] static Result failure(NTSTATUS status, T value) noexcept {
        NT_ASSERT(!NT_SUCCESS(status));
        return Result{status, value};
    }

    /// Whether the native operation succeeded.
    [[nodiscard]] constexpr bool has_value() const noexcept { return NT_SUCCESS(status_); }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return has_value(); }

    /// Access the value after checking that the result succeeded.
    [[nodiscard]] constexpr T& value() noexcept {
        NT_ASSERT(has_value());
        return value_;
    }
    [[nodiscard]] constexpr const T& value() const noexcept {
        NT_ASSERT(has_value());
        return value_;
    }

    [[nodiscard]] constexpr T& operator*() noexcept { return value(); }
    [[nodiscard]] constexpr const T& operator*() const noexcept { return value(); }
    [[nodiscard]] constexpr T* operator->() noexcept { return &value(); }
    [[nodiscard]] constexpr const T* operator->() const noexcept { return &value(); }

    /// Return the native status for either success or failure.
    [[nodiscard]] constexpr NTSTATUS status() const noexcept { return status_; }

private:
    constexpr Result(NTSTATUS status, T value) noexcept : status_{status}, value_{value} {}

    NTSTATUS status_;
    T value_;
};

}  // namespace winwrap::kernel::driver
