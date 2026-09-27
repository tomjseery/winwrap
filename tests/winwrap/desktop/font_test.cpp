#include "winwrap/desktop/font.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("font::default_gui is the shared stock GUI font") {
    const HFONT font = winwrap::font::default_gui();

    REQUIRE(font != nullptr);
    CHECK(font == winwrap::font::default_gui());  // a stock object, not a new font per call
    LOGFONTW description{};
    CHECK(GetObjectW(font, sizeof(description), &description) == sizeof(description));
}
