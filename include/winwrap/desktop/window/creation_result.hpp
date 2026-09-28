#pragma once

#include "winwrap/detail/user_mode.hpp"

#include <expected>
#include <memory>
#include <system_error>
#include <utility>

namespace winwrap {

/// Owns either a permanently addressed created object or its creation error.
///
/// Moving this result transfers only the owning pointer; the created object never
/// moves. A default-constructed result is an empty member slot with no error code.
/// Check the result before using operator* or operator->.
///
/// @tparam T  Non-movable object owned at a stable address.
template <typename T>
class CreationResult {
public:
    CreationResult() : result_{std::unexpected{std::error_code{}}} {}

    /// Takes ownership of a successfully created object.
    explicit CreationResult(std::unique_ptr<T> value) : result_{std::move(value)} {}

    /// Stores the error that prevented creation.
    explicit CreationResult(std::error_code error) : result_{std::unexpected{error}} {}

    /// Whether creation produced an object.
    [[nodiscard]] bool has_value() const noexcept {
        return result_.has_value() && result_.value() != nullptr;
    }

    /// Whether creation produced an object.
    [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }

    /// Returns the creation error; call only when the result has no value.
    [[nodiscard]] std::error_code error() const {
        if (result_.has_value())
            return {};
        return result_.error();
    }

    /// Accesses the created object; throws std::bad_expected_access when unsuccessful.
    [[nodiscard]] T& operator*() { return *operator->(); }

    /// Accesses the created object; throws std::bad_expected_access when unsuccessful.
    [[nodiscard]] const T& operator*() const { return *operator->(); }

    /// Accesses the created object; throws std::bad_expected_access when unsuccessful.
    [[nodiscard]] T* operator->() {
        if (!has_value())
            throw std::bad_expected_access<std::error_code>{error()};
        return result_.value().get();
    }

    /// Accesses the created object; throws std::bad_expected_access when unsuccessful.
    [[nodiscard]] const T* operator->() const {
        if (!has_value())
            throw std::bad_expected_access<std::error_code>{error()};
        return result_.value().get();
    }

private:
    std::expected<std::unique_ptr<T>, std::error_code> result_;
};

}  // namespace winwrap
