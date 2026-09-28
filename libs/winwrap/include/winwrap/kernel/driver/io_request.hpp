#pragma once

#include <concepts>
#include <expected>

#include "winwrap/kernel/driver/detail/wdf.hpp"
#include "winwrap/kernel/memory.hpp"

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
    [[nodiscard]] std::expected<IoRequestBuffer<const T>, NTSTATUS> input() const noexcept {
        void* data{};
        size_t size{};
        const auto status{WdfRequestRetrieveInputBuffer(native_, sizeof(T), &data, &size)};
        if (!NT_SUCCESS(status))
            return std::unexpected{status};
        if (reinterpret_cast<ULONG_PTR>(data) % alignof(T) != 0)
            return std::unexpected{STATUS_DATATYPE_MISALIGNMENT};
        return IoRequestBuffer<const T>{static_cast<const T*>(data), size};
    }

    /// Retrieve an output buffer large enough for one T.
    template <typename T>
    [[nodiscard]] std::expected<IoRequestBuffer<T>, NTSTATUS> output() const noexcept {
        void* data{};
        size_t size{};
        const auto status{WdfRequestRetrieveOutputBuffer(native_, sizeof(T), &data, &size)};
        if (!NT_SUCCESS(status))
            return std::unexpected{status};
        if (reinterpret_cast<ULONG_PTR>(data) % alignof(T) != 0)
            return std::unexpected{STATUS_DATATYPE_MISALIGNMENT};
        return IoRequestBuffer<T>{static_cast<T*>(data), size};
    }

    /// Copy one typed value out of the request's input buffer.
    template <memory::TriviallyCopyable T>
        requires std::default_initializable<T>
    [[nodiscard]] std::expected<T, NTSTATUS> read() const noexcept {
        const auto buffer{input<T>()};
        if (!buffer)
            return std::unexpected{buffer.error()};

        T value{};
        memory::copy(value, *buffer->data());
        return value;
    }

    /// Copy one typed value, or a whole array, into the request's output buffer.
    template <memory::TriviallyCopyable T>
    [[nodiscard]] NTSTATUS write(const T& value) const noexcept {
        const auto buffer{output<T>()};
        if (!buffer)
            return buffer.error();

        memory::copy(*buffer->data(), value);
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
