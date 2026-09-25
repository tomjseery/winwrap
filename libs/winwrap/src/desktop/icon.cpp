#include "winwrap/desktop/icon.hpp"

#include "winwrap/error.hpp"
#include "winwrap/module.hpp"

namespace winwrap {
namespace {

[[nodiscard]] SIZE pixels(IconSize size) noexcept {
    if (size == IconSize::small)
        return {GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON)};
    return {GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON)};
}

// Without LR_SHARED, LoadImageW returns a handle the caller must DestroyIcon.
[[nodiscard]] std::expected<wil::unique_hicon, std::error_code> load(HINSTANCE module,
                                                                     const wchar_t* name,
                                                                     IconSize size, UINT flags) {
    const SIZE extent = pixels(size);
    return check(LoadImageW(module, name, IMAGE_ICON, extent.cx, extent.cy, flags))
        .transform([](HANDLE image) { return wil::unique_hicon{static_cast<HICON>(image)}; });
}

}  // namespace

std::expected<wil::unique_hicon, std::error_code> load_icon(SystemIcon icon, IconSize size) {
    // System icons load only as shared handles (ERROR_RESOURCE_TYPE_NOT_FOUND otherwise),
    // which nobody may destroy, so hand back a private copy at the requested size.
    const SIZE extent = pixels(size);
    return check(LoadImageW(nullptr, MAKEINTRESOURCEW(static_cast<WORD>(icon)), IMAGE_ICON,
                            extent.cx, extent.cy, LR_SHARED))
        .and_then([&](HANDLE shared) {
            return check(CopyImage(shared, IMAGE_ICON, extent.cx, extent.cy, 0));
        })
        .transform([](HANDLE copy) { return wil::unique_hicon{static_cast<HICON>(copy)}; });
}

std::expected<wil::unique_hicon, std::error_code> load_icon(HMODULE module, WORD resource_id,
                                                            IconSize size) {
    if (!module)
        return std::unexpected(win32_error(ERROR_INVALID_HANDLE));
    return load(module, MAKEINTRESOURCEW(resource_id), size, 0);
}

std::expected<wil::unique_hicon, std::error_code> load_icon(WORD resource_id, IconSize size) {
    return load_icon(current_module(), resource_id, size);
}

std::expected<wil::unique_hicon, std::error_code> load_icon(const std::filesystem::path& file,
                                                            IconSize size) {
    return load(nullptr, file.c_str(), size, LR_LOADFROMFILE);
}

}  // namespace winwrap
