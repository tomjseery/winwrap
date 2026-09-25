include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
    GIT_SHALLOW TRUE
    SYSTEM)
FetchContent_MakeAvailable(Catch2)

add_executable(winwrap_test
    device_test.cpp
    desktop/icon_test.cpp
    desktop/notify_icon_test.cpp
    desktop/shell/change_notification_test.cpp
    desktop/window/base_window_test.cpp
    desktop/window/creation_result_test.cpp
    desktop/window/window_test.cpp
    desktop/window/drop_window_test.cpp
    desktop/window/message/file_droppable_test.cpp
    desktop/window/message/timer_tick_test.cpp
    desktop/message_test.cpp
    desktop/window/native_window_test.cpp
    module_test.cpp
    desktop/window/control_test.cpp
    desktop/window/controls/button_test.cpp
    desktop/window/controls/checkbox_test.cpp
    desktop/window/controls/edit_test.cpp
    desktop/window/controls/combobox_test.cpp
    desktop/message_loop_test.cpp
    filesystem/file_attributes_test.cpp)

target_link_libraries(winwrap_test
    PRIVATE
        winwrap::winwrap
        winwrap_warnings
        winwrap_sanitize
        Catch2::Catch2WithMain)

# Device control's injected-call seam is private implementation test support.
target_include_directories(winwrap_test PRIVATE ${PROJECT_SOURCE_DIR}/libs/winwrap/src)

include(Catch)
catch_discover_tests(winwrap_test)

# Compile each public header as its own translation unit, so a header that leans on
# something only another header includes fails the build. It also rejects macros
# leaked by public behavior headers. The glob is deliberate: the check must cover
# every public header, including ones added later, and adds no production sources.
get_target_property(winwrap_source_dir winwrap SOURCE_DIR)
set(winwrap_include_dir ${winwrap_source_dir}/include)
file(GLOB_RECURSE winwrap_public_headers CONFIGURE_DEPENDS
    RELATIVE ${winwrap_include_dir} ${winwrap_include_dir}/winwrap/*.hpp)
set(winwrap_header_units)
foreach(header IN LISTS winwrap_public_headers)
    string(MAKE_C_IDENTIFIER ${header} unit)
    set(unit_source "#include \"${header}\"\n")
    if(NOT header STREQUAL "winwrap/desktop/window/message/detail/hook_case.hpp")
        string(APPEND unit_source
            "#ifdef WW_CASE\n#error \"${header} leaks WW_CASE\"\n#endif\n")
    endif()
    set(unit_path ${CMAKE_CURRENT_BINARY_DIR}/header_units/${unit}.cpp)
    file(CONFIGURE OUTPUT ${unit_path} CONTENT "${unit_source}")
    list(APPEND winwrap_header_units ${unit_path})
endforeach()

add_library(winwrap_header_check OBJECT ${winwrap_header_units})
target_link_libraries(winwrap_header_check
    PRIVATE
        winwrap::winwrap
        winwrap_warnings
        winwrap_sanitize)
add_dependencies(winwrap_test winwrap_header_check)
