#pragma once

#include "winwrap/kernel/driver/result.hpp"

namespace winwrap::kernel::driver {

/// A borrowed typed view of a framework request buffer.
template <typename T>
class IoRequestBuffer final {
public:
    constexpr IoRequestBuffer(T* data, size_t size) noexcept : data_{data}, size_{size} {}

    /// Address of the first typed object in the buffer.
    [[nodiscard]] constexpr T* data() const noexcept { return data_; }

    /// Total native buffer capacity in bytes.
    [[nodiscard]] constexpr size_t size() const noexcept { return size_; }

    [[nodiscard]] constexpr T& operator*() const noexcept { return *data_; }
    [[nodiscard]] constexpr T* operator->() const noexcept { return data_; }

private:
    T* data_;
    size_t size_;
};

/// A borrowed KMDF I/O request. The request becomes invalid when it is completed.
class IoRequest final {
public:
    /// Wrap an I/O request handle delivered by a KMDF callback.
    constexpr explicit IoRequest(WDFREQUEST native) noexcept : native_{native} {}

    /// Retrieve an input buffer large enough for one T.
    template <typename T>
    [[nodiscard]] Result<IoRequestBuffer<const T>> input() const noexcept {
        void* data{};
        size_t size{};
        const auto status{WdfRequestRetrieveInputBuffer(native_, sizeof(T), &data, &size)};
        if (!NT_SUCCESS(status))
            return Result<IoRequestBuffer<const T>>::failure(status,
                                                             IoRequestBuffer<const T>{nullptr, 0});
        if (reinterpret_cast<ULONG_PTR>(data) % alignof(T) != 0)
            return Result<IoRequestBuffer<const T>>::failure(STATUS_DATATYPE_MISALIGNMENT,
                                                             IoRequestBuffer<const T>{nullptr, 0});
        return Result<IoRequestBuffer<const T>>::success(
            IoRequestBuffer<const T>{static_cast<const T*>(data), size});
    }

    /// Retrieve an output buffer large enough for one T.
    template <typename T>
    [[nodiscard]] Result<IoRequestBuffer<T>> output() const noexcept {
        void* data{};
        size_t size{};
        const auto status{WdfRequestRetrieveOutputBuffer(native_, sizeof(T), &data, &size)};
        if (!NT_SUCCESS(status))
            return Result<IoRequestBuffer<T>>::failure(status, IoRequestBuffer<T>{nullptr, 0});
        if (reinterpret_cast<ULONG_PTR>(data) % alignof(T) != 0)
            return Result<IoRequestBuffer<T>>::failure(STATUS_DATATYPE_MISALIGNMENT,
                                                       IoRequestBuffer<T>{nullptr, 0});
        return Result<IoRequestBuffer<T>>::success(IoRequestBuffer<T>{static_cast<T*>(data), size});
    }

    /// Copy one typed value out of the request's input buffer.
    template <typename T>
    [[nodiscard]] Result<T> read() const noexcept {
        static_assert(
            __is_trivially_copyable(T),
            "winwrap::kernel::driver::IoRequest::read requires a trivially copyable type");
        const auto buffer{input<T>()};
        if (!buffer)
            return Result<T>::failure(buffer.status(), T{});

        T value{};
        RtlCopyMemory(&value, buffer->data(), sizeof(T));
        return Result<T>::success(value);
    }

    /// Copy one typed value into the request's output buffer.
    template <typename T>
    [[nodiscard]] NTSTATUS write(const T& value) const noexcept {
        static_assert(
            __is_trivially_copyable(T),
            "winwrap::kernel::driver::IoRequest::write requires a trivially copyable type");
        const auto buffer{output<T>()};
        if (!buffer)
            return buffer.status();

        RtlCopyMemory(buffer->data(), &value, sizeof(T));
        return STATUS_SUCCESS;
    }

    /// Complete the request. The request and all retrieved buffers are invalid afterward.
    void complete(NTSTATUS status, ULONG_PTR information = 0) const noexcept {
        WdfRequestCompleteWithInformation(native_, status, information);
    }

    /// Borrow the native KMDF handle before completion.
    [[nodiscard]] constexpr WDFREQUEST native() const noexcept { return native_; }

private:
    WDFREQUEST native_;
};

}  // namespace winwrap::kernel::driver
