#include "winwrap/desktop/shell/change_notification.hpp"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "winwrap/desktop/message_loop.hpp"
#include "winwrap/desktop/shell/change_registration.hpp"
#include "winwrap/desktop/window/message/shell_change_aware.hpp"
#include "winwrap/desktop/window/window.hpp"

using namespace std::chrono_literals;

namespace {

constexpr UINT_PTR timeout_timer{1};

struct ShellChange {
    LONG event{};
    std::optional<std::filesystem::path> path;
    std::optional<std::filesystem::path> new_path;
};

// Records the Shell changes delivered to it and stops the loop on the expected one.
struct ChangeWatcher : winwrap::Window<ChangeWatcher, winwrap::ShellChangeAware> {
    static constexpr const wchar_t* class_name = L"WinwrapShellChangeWatcher";
    std::vector<ShellChange> changes;
    LONG expected{};

    void on_shell_change(const winwrap::shell::ChangeNotification& change) {
        changes.push_back({change.event(), change.path(), change.new_path()});
        if (change.event() == expected)
            winwrap::message_loop::quit();
    }

    void on_timer(UINT_PTR) { winwrap::message_loop::quit(); }  // give up waiting

    // Runs `action` and pumps messages until the Shell reports `event` or 5 s pass.
    std::optional<ShellChange> delivered(LONG event, const std::function<void()>& action) {
        changes.clear();
        expected = event;
        action();
        REQUIRE(start_timer(timeout_timer, 5s));
        static_cast<void>(winwrap::message_loop::run());
        static_cast<void>(stop_timer(timeout_timer));
        const auto found = std::ranges::find(changes, event, &ShellChange::event);
        if (found == changes.end())
            return std::nullopt;
        return *found;
    }
};

std::filesystem::path make_temp_folder() {
    auto folder = std::filesystem::temp_directory_path() /
                  (L"winwrap_shell_test_" + std::to_wstring(GetCurrentProcessId()));
    std::filesystem::remove_all(folder);
    std::filesystem::create_directory(folder);
    return std::filesystem::canonical(folder);
}

std::filesystem::path short_path(const std::filesystem::path& path) {
    std::wstring shortened(GetShortPathNameW(path.c_str(), nullptr, 0), L'\0');
    shortened.resize(
        GetShortPathNameW(path.c_str(), shortened.data(), static_cast<DWORD>(shortened.size())));
    return shortened;
}

}  // namespace

// Needs a running Explorer: SHChangeNotify events are routed through the Shell.
TEST_CASE("shell notifications reach a registered Shell listener") {
    const auto folder = make_temp_folder();
    auto watcher = ChangeWatcher::create({.parent = HWND_MESSAGE});
    REQUIRE(watcher);
    auto& w = *watcher;
    {
        const auto registration = winwrap::shell::ChangeRegistration::create({
            .owner = w.hwnd(),
            .folder = folder,
            .sources = SHCNRF_ShellLevel,
        });
        REQUIRE(registration);
        const auto file = folder / L"created.txt";
        const auto renamed = folder / L"renamed.txt";
        const auto subfolder = folder / L"sub";

        const auto created = w.delivered(SHCNE_CREATE, [&] {
            std::ofstream{file} << "x";
            winwrap::shell::notify_file_created(file);
        });
        REQUIRE(created);
        CHECK(created->path == file);
        CHECK_FALSE(created->new_path);

        const auto changed = w.delivered(SHCNE_UPDATEITEM, [&] {
            std::ofstream{file} << "changed";
            winwrap::shell::notify_file_changed(file);
        });
        REQUIRE(changed);
        CHECK(changed->path == file);

        const auto moved = w.delivered(SHCNE_RENAMEITEM, [&] {
            std::filesystem::rename(file, renamed);
            winwrap::shell::notify_file_renamed(file, renamed);
        });
        REQUIRE(moved);
        CHECK(moved->path == file);
        CHECK(moved->new_path == renamed);

        const auto deleted = w.delivered(SHCNE_DELETE, [&] {
            std::filesystem::remove(renamed);
            winwrap::shell::notify_file_deleted(renamed);
        });
        REQUIRE(deleted);
        CHECK(deleted->path == renamed);

        const auto made = w.delivered(SHCNE_MKDIR, [&] {
            std::filesystem::create_directory(subfolder);
            winwrap::shell::notify_folder_created(subfolder);
        });
        REQUIRE(made);
        CHECK(made->path == subfolder);

        const auto removed = w.delivered(SHCNE_RMDIR, [&] {
            std::filesystem::remove(subfolder);
            winwrap::shell::notify_folder_deleted(subfolder);
        });
        REQUIRE(removed);
        CHECK(removed->path == subfolder);
    }
    std::filesystem::remove_all(folder);
}

TEST_CASE("a notification naming an 8.3 path reaches the listener as the long path") {
    const auto folder = make_temp_folder();
    const auto file = folder / L"long_file_name.txt";
    std::ofstream{file} << "x";
    const auto shortened = short_path(file);
    if (shortened == file)
        SKIP("this volume creates no 8.3 names");
    auto watcher = ChangeWatcher::create({.parent = HWND_MESSAGE});
    REQUIRE(watcher);
    {
        const auto registration = winwrap::shell::ChangeRegistration::create({
            .owner = watcher->hwnd(),
            .folder = folder,
            .sources = SHCNRF_ShellLevel,
        });
        REQUIRE(registration);

        const auto changed = watcher->delivered(
            SHCNE_UPDATEITEM, [&] { winwrap::shell::notify_file_changed(shortened); });

        REQUIRE(changed);
        CHECK(changed->path == file);
    }
    std::filesystem::remove_all(folder);
}

TEST_CASE("ChangeRegistration rejects a null owner") {
    const auto registration = winwrap::shell::ChangeRegistration::create({
        .folder = std::filesystem::temp_directory_path(),
    });

    REQUIRE_FALSE(registration);
    CHECK(registration.error().value() == ERROR_INVALID_WINDOW_HANDLE);
}

TEST_CASE("ChangeRegistration reports a folder the Shell cannot find") {
    auto watcher = ChangeWatcher::create({.parent = HWND_MESSAGE});
    REQUIRE(watcher);
    const auto missing = std::filesystem::temp_directory_path() /
                         (L"winwrap_missing_" + std::to_wstring(GetCurrentProcessId()));
    std::filesystem::remove_all(missing);

    const auto registration = winwrap::shell::ChangeRegistration::create({
        .owner = watcher->hwnd(),
        .folder = missing,
    });

    REQUIRE_FALSE(registration);
    CHECK(registration.error().value() == ERROR_FILE_NOT_FOUND);
}

TEST_CASE("a moved ChangeRegistration transfers its registration") {
    auto watcher = ChangeWatcher::create({.parent = HWND_MESSAGE});
    REQUIRE(watcher);
    auto registration = winwrap::shell::ChangeRegistration::create({
        .owner = watcher->hwnd(),
        .folder = std::filesystem::temp_directory_path(),
    });
    REQUIRE(registration);
    const ULONG id = registration->id();
    REQUIRE(id != 0);

    const auto moved = std::move(*registration);

    CHECK(moved.id() == id);
    CHECK(registration->id() == 0);
}

TEST_CASE("ChangeNotification::message is one registered id") {
    const auto message = winwrap::shell::ChangeNotification::message();

    REQUIRE(message);
    CHECK(*message >= 0xC000);
    CHECK(winwrap::shell::ChangeNotification::message() == message);
}

TEST_CASE("ChangeNotification::lock rejects a message that carries no change") {
    const auto change = winwrap::shell::ChangeNotification::lock(0, GetCurrentProcessId());

    CHECK_FALSE(change);
}
