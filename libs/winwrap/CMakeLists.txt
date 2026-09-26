add_library(winwrap STATIC
    src/device.cpp
    src/module.cpp
    src/desktop/icon.cpp
    src/desktop/notify_icon.cpp
    src/desktop/window/native_window.cpp
    src/desktop/menu.cpp)
add_library(winwrap::winwrap ALIAS winwrap)

add_library(winwrap_user INTERFACE)
add_library(winwrap::user ALIAS winwrap_user)
target_link_libraries(winwrap_user INTERFACE winwrap)

add_library(winwrap_kernel INTERFACE)
add_library(winwrap::kernel ALIAS winwrap_kernel)
target_compile_features(winwrap_kernel INTERFACE cxx_std_20)
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

install(TARGETS winwrap winwrap_user winwrap_kernel EXPORT winwrap-targets)
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
