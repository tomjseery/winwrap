#pragma once

#include <ntddk.h>

// KMDF 1.31 forward-declares an enum without its underlying type.
#pragma warning(push)
#pragma warning(disable : 4471)
#include <wdf.h>
#pragma warning(pop)
