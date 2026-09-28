cmake_minimum_required(VERSION 3.28)

project(winwrap
    VERSION 0.1.0
    DESCRIPTION "Modern-C++ CRTP wrappers over Win32 (windows + tray icon)"
    LANGUAGES CXX)

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    include(CTest)
endif()

# Warning flags gated by compiler (MSVC rejects GNU-style options).
add_library(winwrap_warnings INTERFACE)
target_compile_options(winwrap_warnings INTERFACE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra;-Wpedantic;-Wshadow;-Wconversion;-Wsign-conversion>
    $<$<CXX_COMPILER_ID:MSVC>:/W4;/permissive->)

option(WINWRAP_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
if(WINWRAP_WARNINGS_AS_ERRORS)
    target_compile_options(winwrap_warnings INTERFACE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Werror>
        $<$<CXX_COMPILER_ID:MSVC>:/WX>)
endif()

# Opt-in sanitizers: GNU/Clang get ASan+UBSan, MSVC gets ASan (no UBSan).
option(WINWRAP_SANITIZE "Build with AddressSanitizer (and UBSan where available)" OFF)
add_library(winwrap_sanitize INTERFACE)
if(WINWRAP_SANITIZE)
    target_compile_options(winwrap_sanitize INTERFACE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fsanitize=address,undefined;-fno-omit-frame-pointer>
        $<$<CXX_COMPILER_ID:MSVC>:/fsanitize=address>)
    target_link_options(winwrap_sanitize INTERFACE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fsanitize=address,undefined>)
    # MSVC ASan annotates STL containers; Catch2 isn't built with ASan, so the
    # two disagree and the link fails (LNK2038). Turn the annotations off to mix.
    target_compile_definitions(winwrap_sanitize INTERFACE
        $<$<CXX_COMPILER_ID:MSVC>:_DISABLE_STRING_ANNOTATION=1;_DISABLE_VECTOR_ANNOTATION=1>)
endif()

include(FetchContent)
FetchContent_Declare(wil
    GIT_REPOSITORY https://github.com/microsoft/wil.git
    GIT_TAG v1.0.260126.7
    GIT_SHALLOW TRUE
    SYSTEM)
set(WIL_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(WIL_BUILD_PACKAGING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(wil)

add_library(winwrap STATIC
    src/device.cpp
    src/disk_serial.cpp
    src/firmware.cpp
    src/system_uuid.cpp
    src/module.cpp
    src/desktop/icon.cpp
    src/desktop/notify_icon.cpp
    src/desktop/menu.cpp)
add_library(winwrap::winwrap ALIAS winwrap)

add_library(winwrap_kernel INTERFACE)
set_target_properties(winwrap_kernel PROPERTIES EXPORT_NAME kernel)
add_library(winwrap::kernel ALIAS winwrap_kernel)
target_compile_features(winwrap_kernel INTERFACE cxx_std_23)
target_include_directories(winwrap_kernel
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_compile_features(winwrap PUBLIC cxx_std_23)

# Public headers: this path while building from the tree, the installed path
# (just `include`) after `cmake --install`.
target_include_directories(winwrap
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

# Unicode Win32 + lean headers, propagated to anything that compiles against us.
target_compile_definitions(winwrap PUBLIC UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(winwrap PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

target_link_libraries(winwrap
    PUBLIC
        user32
        shell32
        comctl32
        cfgmgr32
        gdi32
        $<BUILD_INTERFACE:WIL::WIL>
    PRIVATE
        $<BUILD_INTERFACE:winwrap_warnings>
        $<BUILD_INTERFACE:winwrap_sanitize>)

# --- install / export: lets a consumer `find_package(winwrap)` after install ---
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS winwrap winwrap_kernel EXPORT winwrap-targets)
install(EXPORT winwrap-targets
    FILE winwrap-targets.cmake
    NAMESPACE winwrap::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/winwrap)

configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/winwrap-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/winwrap-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/winwrap)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/winwrap-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/winwrap-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/winwrap-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/winwrap)

if(PROJECT_IS_TOP_LEVEL AND BUILD_TESTING)
    add_subdirectory(tests)
endif()
