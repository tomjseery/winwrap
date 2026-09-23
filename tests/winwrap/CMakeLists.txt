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
    window_test.cpp
    drop_window_test.cpp
    file_droppable_test.cpp
    control_test.cpp
    button_test.cpp
    checkbox_test.cpp
    edit_test.cpp
    combobox_test.cpp
    message_loop_test.cpp
    file_attributes_test.cpp)

target_link_libraries(winwrap_test
    PRIVATE
        winwrap::winwrap
        winwrap_warnings
        winwrap_sanitize
        Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(winwrap_test)

# Compile each public header as its own translation unit, so a header that leans on
# something only another header includes fails the build. A hook mixin must
# also define WINWRAP_HOOK_CASE itself: a missing macro would otherwise surface only
# when a template is instantiated. The glob is deliberate: the check must cover every
# public header, including ones added later, and adds no production sources.
get_target_property(winwrap_source_dir winwrap SOURCE_DIR)
set(winwrap_include_dir ${winwrap_source_dir}/include)
file(GLOB_RECURSE winwrap_public_headers CONFIGURE_DEPENDS
    RELATIVE ${winwrap_include_dir} ${winwrap_include_dir}/winwrap/*.hpp)
set(winwrap_header_units)
foreach(header IN LISTS winwrap_public_headers)
    string(MAKE_C_IDENTIFIER ${header} unit)
    set(unit_source "#include \"${header}\"\n")
    file(READ ${winwrap_include_dir}/${header} header_text)
    string(FIND "${header_text}" "WINWRAP_HOOK_CASE(WM_" hook_use)
    if(NOT hook_use EQUAL -1)
        string(APPEND unit_source
            "#ifndef WINWRAP_HOOK_CASE\n#error \"${header} must define WINWRAP_HOOK_CASE\"\n#endif\n")
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
