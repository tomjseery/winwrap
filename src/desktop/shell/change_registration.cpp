#include "winwrap/desktop/shell/change_registration.hpp"

#include "winwrap/desktop/shell/change_notification.hpp"
#include "winwrap/error.hpp"

namespace winwrap::shell {
namespace {

using unique_item_list = wil::unique_any<PIDLIST_ABSOLUTE, decltype(&::ILFree), ::ILFree>;

[[nodiscard]] std::error_code hresult_error(HRESULT result) {
    if (HRESULT_FACILITY(result) == FACILITY_WIN32)
        return error::win32(HRESULT_CODE(result));
    return std::error_code{static_cast<int>(result), std::system_category()};
}

}  // namespace

std::expected<ChangeRegistration, std::error_code> ChangeRegistration::create(
    const ChangeRegistrationConfig& config) {
    if (!config.owner)
        return std::unexpected(error::win32(ERROR_INVALID_WINDOW_HANDLE));
    const auto message{ChangeNotification::message()};
    if (!message)
        return std::unexpected(message.error());

    PIDLIST_ABSOLUTE parsed{};
    const HRESULT parsing{
        ::SHParseDisplayName(config.folder.c_str(), nullptr, &parsed, 0, nullptr)};
    const unique_item_list folder{parsed};
    if (FAILED(parsing))
        return std::unexpected(hresult_error(parsing));

    int sources{config.sources | SHCNRF_NewDelivery};
    if (config.recursive && (config.sources & SHCNRF_InterruptLevel) != 0)
        sources |= SHCNRF_RecursiveInterrupt;
    const SHChangeNotifyEntry entry{.pidl = folder.get(),
                                    .fRecursive = config.recursive ? TRUE : FALSE};

    return error::result_or_last([&] {
               return ::SHChangeNotifyRegister(config.owner, sources, config.events, *message, 1,
                                               &entry);
           })
        .and_then([](ULONG id) -> std::expected<ChangeRegistration, std::error_code> {
            if (id == 0)
                return std::unexpected(error::win32(ERROR_GEN_FAILURE));
            return ChangeRegistration{unique_registration{id}};
        });
}

}  // namespace winwrap::shell
