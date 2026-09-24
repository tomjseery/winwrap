// Deliberately repeatable internal include fragment. Each hook behavior includes it
// before its definition and undefines WW_CASE afterwards, so the implementation macro
// never leaks through a public behavior header. Macros ignore C++ namespaces; WW is the
// library prefix. This detail header is reachable only because public templates need it;
// direct consumer inclusion is unsupported.
#if defined(WW_CASE)
#error "winwrap's internal WW_CASE support is already active"
#endif

// One message case: call the hook iff the final type defines it -- existence is
// detected in an unevaluated `requires` and resolved by `if constexpr`, so the
// missing-hook branch emits no code (zero runtime cost). Deriving the `requires`
// check from the same `call` makes the two impossible to drift apart.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define WW_CASE(message, call)             \
    case message:                           \
        if constexpr (requires { call; }) { \
            call;                           \
            return 0;                       \
        } else                              \
            break
