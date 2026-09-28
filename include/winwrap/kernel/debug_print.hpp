#pragma once

#ifndef _KERNEL_MODE
#error WinWrap kernel debug printing requires kernel mode
#endif

#include <ntddk.h>

namespace winwrap::kernel {

template <typename... Args>
ULONG debug_print(ULONG component_id, ULONG level, _Printf_format_string_ PCSTR format,
                  Args... args) noexcept {
    return DbgPrintEx(component_id, level, format, args...);
}

}  // namespace winwrap::kernel
