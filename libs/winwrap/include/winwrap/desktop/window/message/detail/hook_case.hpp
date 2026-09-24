#pragma once

// One message case: call the hook iff the final type defines it -- existence is
// detected in an unevaluated `requires` and resolved by `if constexpr`, so the
// missing-hook branch emits no code (zero runtime cost). Deriving the `requires`
// check from the same `call` makes the two impossible to drift apart. Each hook behavior
// includes this header itself so it compiles on its own. Macros ignore C++ namespaces;
// WW is the library prefix.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define WW_CASE(message, call)             \
    case message:                           \
        if constexpr (requires { call; }) { \
            call;                           \
            return 0;                       \
        } else                              \
            break
