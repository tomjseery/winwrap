#include "winwrap/desktop/cursor.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("cursor::position reports the cursor in screen coordinates") {
    POINT native{};
    REQUIRE(GetCursorPos(&native));

    const auto position = winwrap::cursor::position();

    REQUIRE(position);
    CHECK(position->x == native.x);
    CHECK(position->y == native.y);
}
