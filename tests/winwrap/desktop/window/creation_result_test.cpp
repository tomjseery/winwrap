#include "winwrap/user/desktop/window/creation_result.hpp"

#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <system_error>
#include <type_traits>
#include <utility>

namespace {
struct CreatedObject {
    int value{42};
};
}  // namespace

static_assert(!std::is_copy_constructible_v<winwrap::user::CreationResult<CreatedObject>>);
static_assert(std::is_move_constructible_v<winwrap::user::CreationResult<CreatedObject>>);

TEST_CASE("CreationResult directly accesses its owned object") {
    winwrap::user::CreationResult<CreatedObject> result{std::make_unique<CreatedObject>()};

    REQUIRE(result);
    CHECK(result->value == 42);
    CHECK((*result).value == 42);
}

TEST_CASE("CreationResult carries a creation error") {
    const auto error = std::make_error_code(std::errc::invalid_argument);
    const winwrap::user::CreationResult<CreatedObject> result{error};

    REQUIRE_FALSE(result);
    CHECK(result.error() == error);
}

TEST_CASE("moving CreationResult transfers ownership without moving the object") {
    winwrap::user::CreationResult<CreatedObject> source{std::make_unique<CreatedObject>()};
    const auto* address = &*source;

    auto destination = std::move(source);

    REQUIRE(destination);
    CHECK(&*destination == address);
    CHECK_FALSE(source);
    CHECK_FALSE(source.error());
}

TEST_CASE("default CreationResult is an empty member slot") {
    const winwrap::user::CreationResult<CreatedObject> result;

    CHECK_FALSE(result);
    CHECK_FALSE(result.error());
}
