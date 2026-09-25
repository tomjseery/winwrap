#include "winwrap/desktop/shell/change_notification.hpp"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <vector>

#include "winwrap/desktop/message_loop.hpp"
#include "winwrap/desktop/window/window.hpp"

using namespace std::chrono_literals;

namespace {

constexpr UINT change_message{WM_APP + 9};
constexpr UINT_PTR timeout_timer{1};

// Records the Shell events delivered for a watched folder, using the raw registration
// API (the listening half is not wrapped yet), and stops the loop on the expected one.
struct ChangeWatcher : winwrap::Window<ChangeWatcher> {
    static constexpr const wchar_t* class_name = L"WinwrapShellChangeWatcher";
    std::vector<LONG> events;
    LONG expected{};

    LRESULT route_message(UINT msg, WPARAM wparam, LPARAM lparam) {
        if (msg == change_message) {
            PIDLIST_ABSOLUTE* items{};
            LONG event{};
            if (HANDLE lock = SHChangeNotification_Lock(
                    reinterpret_cast<HANDLE>(wparam), static_cast<DWORD>(lparam), &items, &event)) {
                events.push_back(event);
                SHChangeNotification_Unlock(lock);
            }
            if (event == expected)
                winwrap::message_loop::quit();
            return 0;
        }
        return Window::route_message(msg, wparam, lparam);
    }

    void on_timer(UINT_PTR) { winwrap::message_loop::quit(); }  // give up waiting

    // Runs `action` and pumps messages until the Shell reports `event` or 5 s pass.
    bool delivers(LONG event, const std::function<void()>& action) {
        events.clear();
        expected = event;
        action();
        REQUIRE(start_timer(timeout_timer, 5s));
        static_cast<void>(winwrap::message_loop::run());
        static_cast<void>(stop_timer(timeout_timer));
        return std::ranges::find(events, event) != events.end();
    }
};

// Watches `folder` for Shell-level events for the lifetime of the object.
class Registration {
public:
    Registration(HWND hwnd, const std::filesystem::path& folder)
        : folder_{ILCreateFromPathW(folder.c_str())} {
        REQUIRE(folder_ != nullptr);
        const SHChangeNotifyEntry entry{folder_, FALSE};
        id_ = SHChangeNotifyRegister(hwnd, SHCNRF_ShellLevel | SHCNRF_NewDelivery, SHCNE_ALLEVENTS,
                                     change_message, 1, &entry);
        REQUIRE(id_ != 0);
    }
    ~Registration() {
        SHChangeNotifyDeregister(id_);
        ILFree(folder_);
    }
    Registration(const Registration&) = delete;
    Registration& operator=(const Registration&) = delete;

private:
    PIDLIST_ABSOLUTE folder_;
    ULONG id_{};
};

std::filesystem::path make_temp_folder() {
    auto folder = std::filesystem::temp_directory_path() /
                  (L"winwrap_shell_test_" + std::to_wstring(GetCurrentProcessId()));
    std::filesystem::remove_all(folder);
    std::filesystem::create_directory(folder);
    return folder;
}

}  // namespace

// Needs a running Explorer: SHChangeNotify events are routed through the Shell.
TEST_CASE("shell notifications reach a registered Shell listener") {
    const auto folder = make_temp_folder();
    auto watcher = ChangeWatcher::create({.parent = HWND_MESSAGE});
    REQUIRE(watcher);
    auto& w = *watcher;
    {
        Registration registration{w.hwnd(), folder};
        const auto file = folder / L"created.txt";
        const auto renamed = folder / L"renamed.txt";
        const auto subfolder = folder / L"sub";

        CHECK(w.delivers(SHCNE_CREATE, [&] {
            std::ofstream{file} << "x";
            winwrap::shell::notify_file_created(file);
        }));
        CHECK(w.delivers(SHCNE_UPDATEITEM, [&] {
            std::ofstream{file} << "changed";
            winwrap::shell::notify_file_changed(file);
        }));
        CHECK(w.delivers(SHCNE_RENAMEITEM, [&] {
            std::filesystem::rename(file, renamed);
            winwrap::shell::notify_file_renamed(file, renamed);
        }));
        CHECK(w.delivers(SHCNE_DELETE, [&] {
            std::filesystem::remove(renamed);
            winwrap::shell::notify_file_deleted(renamed);
        }));
        CHECK(w.delivers(SHCNE_MKDIR, [&] {
            std::filesystem::create_directory(subfolder);
            winwrap::shell::notify_folder_created(subfolder);
        }));
        CHECK(w.delivers(SHCNE_RMDIR, [&] {
            std::filesystem::remove(subfolder);
            winwrap::shell::notify_folder_deleted(subfolder);
        }));
    }
    std::filesystem::remove_all(folder);
}
