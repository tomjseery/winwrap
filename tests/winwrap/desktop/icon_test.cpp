#include "winwrap/user/desktop/icon.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "winwrap/user/module.hpp"

namespace {

// The loaded icon's pixel size, read back through GetIconInfo.
SIZE icon_extent(HICON icon) {
    ICONINFO info{};
    REQUIRE(GetIconInfo(icon, &info));
    wil::unique_hbitmap mask{info.hbmMask};
    wil::unique_hbitmap color{info.hbmColor};
    BITMAP bitmap{};
    REQUIRE(GetObjectW(color.get(), sizeof(bitmap), &bitmap) != 0);
    return {bitmap.bmWidth, bitmap.bmHeight};
}

SIZE small_icon_metric() {
    return {GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON)};
}

void put(std::vector<char>& out, std::uint32_t value, int bytes) {
    for (int i = 0; i < bytes; ++i)
        out.push_back(static_cast<char>((value >> (8 * i)) & 0xFF));
}

// Writes a minimal single-image 16x16, 32-bit .ico file.
void write_ico(const std::filesystem::path& file) {
    constexpr int side{16};
    constexpr std::uint32_t pixel_bytes{side * side * 4};
    constexpr std::uint32_t mask_bytes{side * 4};  // 1-bit rows padded to 32 bits
    constexpr std::uint32_t header_bytes{40};
    std::vector<char> ico;
    put(ico, 0, 2);  // ICONDIR: reserved, type 1 (icon), one image
    put(ico, 1, 2);
    put(ico, 1, 2);
    put(ico, side, 1);  // ICONDIRENTRY
    put(ico, side, 1);
    put(ico, 0, 1);
    put(ico, 0, 1);
    put(ico, 1, 2);
    put(ico, 32, 2);
    put(ico, header_bytes + pixel_bytes + mask_bytes, 4);
    put(ico, 6 + 16, 4);
    put(ico, header_bytes, 4);  // BITMAPINFOHEADER; height counts colour + mask
    put(ico, side, 4);
    put(ico, side * 2, 4);
    put(ico, 1, 2);
    put(ico, 32, 2);
    put(ico, 0, 4);
    put(ico, pixel_bytes + mask_bytes, 4);
    put(ico, 0, 4);
    put(ico, 0, 4);
    put(ico, 0, 4);
    put(ico, 0, 4);
    for (std::uint32_t i = 0; i < side * side; ++i)
        put(ico, 0xFF3366CC, 4);
    ico.insert(ico.end(), mask_bytes, '\0');

    std::ofstream out{file, std::ios::binary};
    out.write(ico.data(), static_cast<std::streamsize>(ico.size()));
}

std::filesystem::path unique_temp_path(const wchar_t* stem) {
    return std::filesystem::temp_directory_path() /
           (std::wstring{stem} + std::to_wstring(GetCurrentProcessId()) + L".ico");
}

}  // namespace

TEST_CASE("icon::load loads a system icon at the requested metric") {
    auto icon = winwrap::user::icon::load(winwrap::user::SystemIcon::application, winwrap::user::IconSize::small);

    REQUIRE(icon);
    const SIZE extent = icon_extent(icon->get());
    CHECK(extent.cx == small_icon_metric().cx);
    CHECK(extent.cy == small_icon_metric().cy);
}

TEST_CASE("icon::load returns a private copy the caller can destroy") {
    auto first = winwrap::user::icon::load(winwrap::user::SystemIcon::warning, winwrap::user::IconSize::large);
    auto second = winwrap::user::icon::load(winwrap::user::SystemIcon::warning, winwrap::user::IconSize::large);
    REQUIRE(first);
    REQUIRE(second);

    CHECK(first->get() != second->get());
    CHECK(DestroyIcon(first->release()));  // owned, not shared
}

TEST_CASE("icon::load loads an icon resource from a module") {
    // user32.dll ships icon resource 100 (the application icon).
    const auto user32 = winwrap::user::module::loaded(L"user32.dll");
    REQUIRE(user32);
    auto icon = winwrap::user::icon::load(*user32, 100, winwrap::user::IconSize::small);

    REQUIRE(icon);
    CHECK(icon_extent(icon->get()).cx == small_icon_metric().cx);
}

TEST_CASE("icon::load reports a missing resource in the running executable") {
    auto icon = winwrap::user::icon::load(0x7FFF, winwrap::user::IconSize::small);

    REQUIRE_FALSE(icon);
    CHECK(icon.error().value() == ERROR_RESOURCE_TYPE_NOT_FOUND);
}

TEST_CASE("icon::load rejects a null module instead of loading a system icon") {
    auto icon = winwrap::user::icon::load(nullptr, 32512, winwrap::user::IconSize::small);

    REQUIRE_FALSE(icon);
    CHECK(icon.error().value() == ERROR_INVALID_HANDLE);
}

TEST_CASE("icon::load loads an .ico file") {
    const auto file = unique_temp_path(L"winwrap_icon_test_");
    write_ico(file);

    auto icon = winwrap::user::icon::load(file, winwrap::user::IconSize::small);

    std::filesystem::remove(file);
    REQUIRE(icon);
    CHECK(icon_extent(icon->get()).cx == small_icon_metric().cx);
}

TEST_CASE("icon::load reports a missing .ico file") {
    auto icon =
        winwrap::user::icon::load(unique_temp_path(L"winwrap_missing_icon_"), winwrap::user::IconSize::small);

    REQUIRE_FALSE(icon);
    CHECK(icon.error().value() == ERROR_FILE_NOT_FOUND);
}
