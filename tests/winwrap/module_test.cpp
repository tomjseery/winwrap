#include "winwrap/user/module.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("module::current is the running executable") {
    const HMODULE module = winwrap::user::module::current();

    REQUIRE(module != nullptr);
    const auto path = winwrap::user::module::path(module);
    REQUIRE(path);
    CHECK(path->filename() == L"winwrap_test.exe");
}

TEST_CASE("module::loaded finds an already-loaded DLL") {
    const auto user32 = winwrap::user::module::loaded(L"user32.dll");

    REQUIRE(user32);
    const auto path = winwrap::user::module::path(*user32);
    REQUIRE(path);
    CHECK(_wcsicmp(path->filename().c_str(), L"user32.dll") == 0);
}

TEST_CASE("module::loaded reports a module that is not loaded") {
    const auto missing = winwrap::user::module::loaded(L"winwrap-no-such-module.dll");

    REQUIRE_FALSE(missing);
    CHECK(missing.error().value() == ERROR_MOD_NOT_FOUND);
}

TEST_CASE("module::path rejects a null module instead of reporting the executable") {
    const auto path = winwrap::user::module::path(nullptr);

    REQUIRE_FALSE(path);
    CHECK(path.error().value() == ERROR_INVALID_HANDLE);
}

TEST_CASE("Module delegates to the free functions") {
    const auto current = winwrap::user::Module::current();
    const auto user32 = winwrap::user::Module::loaded(L"user32.dll");

    CHECK(current.handle() == winwrap::user::module::current());
    REQUIRE(user32);
    CHECK(user32->handle() == *winwrap::user::module::loaded(L"user32.dll"));
    CHECK(current.path() == winwrap::user::module::path(current.handle()));
    CHECK(winwrap::user::Module{current.handle()} == current);
    CHECK_FALSE(winwrap::user::Module::loaded(L"winwrap-no-such-module.dll"));
}
