#include <catch2/catch_test_macros.hpp>

#include "winwrap/module.hpp"

TEST_CASE("current_module is the running executable") {
    const HMODULE module = winwrap::current_module();

    REQUIRE(module != nullptr);
    const auto path = winwrap::module_path(module);
    REQUIRE(path);
    CHECK(path->filename() == L"winwrap_test.exe");
}

TEST_CASE("loaded_module finds an already-loaded DLL") {
    const auto user32 = winwrap::loaded_module(L"user32.dll");

    REQUIRE(user32);
    const auto path = winwrap::module_path(*user32);
    REQUIRE(path);
    CHECK(_wcsicmp(path->filename().c_str(), L"user32.dll") == 0);
}

TEST_CASE("loaded_module reports a module that is not loaded") {
    const auto missing = winwrap::loaded_module(L"winwrap-no-such-module.dll");

    REQUIRE_FALSE(missing);
    CHECK(missing.error().value() == ERROR_MOD_NOT_FOUND);
}

TEST_CASE("module_path rejects a null module instead of reporting the executable") {
    const auto path = winwrap::module_path(nullptr);

    REQUIRE_FALSE(path);
    CHECK(path.error().value() == ERROR_INVALID_HANDLE);
}

TEST_CASE("Module delegates to the free functions") {
    const auto current = winwrap::Module::current();
    const auto user32 = winwrap::Module::loaded(L"user32.dll");

    CHECK(current.handle() == winwrap::current_module());
    REQUIRE(user32);
    CHECK(user32->handle() == *winwrap::loaded_module(L"user32.dll"));
    CHECK(current.path() == winwrap::module_path(current.handle()));
    CHECK(winwrap::Module{current.handle()} == current);
    CHECK_FALSE(winwrap::Module::loaded(L"winwrap-no-such-module.dll"));
}
