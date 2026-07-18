#ifndef QUIRE_ATTRIBUTES_H_
#define QUIRE_ATTRIBUTES_H_

// Portable wrappers around GCC/Clang function attributes used across the
// project. On compilers that don't support __attribute__ (e.g. MSVC), these
// expand to nothing: the project loses the extra checking/optimization but
// still compiles correctly.

#if defined(__GNUC__) || defined(__clang__)

// Marks a variadic function as printf-like so -Wformat can check callers'
// format strings/arguments against it. fmtIndex/argIndex are 1-based
// parameter positions (argIndex is 0 for functions using va_list directly,
// e.g. vprintf-style).
#define QUIRE_PRINTF(fmtIndex, argIndex) __attribute__((format(printf, fmtIndex, argIndex)))

// Function's result depends only on its arguments; it reads no memory other
// than its arguments (not even through pointer arguments) and has no
// observable side effects. Stronger than QUIRE_PURE — permits the compiler
// to common-subexpression-eliminate or hoist repeated calls with identical
// arguments. Do not use on functions that dereference pointer parameters.
#define QUIRE_CONST __attribute__((const))

// Function has no observable side effects and its result depends only on its
// arguments and/or memory reachable through them; safe to elide/merge
// repeated calls as long as that memory hasn't changed in between. Weaker
// than QUIRE_CONST — use this when the function dereferences pointer
// parameters.
#define QUIRE_PURE __attribute__((pure))

// Warns if the caller discards this function's return value. Intended for
// status/result codes where ignoring the return means silently missing an
// error.
#define QUIRE_WARN_UNUSED_RESULT __attribute__((warn_unused_result))

// Casts away const on `ptr`, narrowly and deliberately, for the rare cases
// where an external API is not const-correct (e.g. Xlib's XImage.data is
// declared as plain char*, even though XPutImage only ever reads through it)
// but is known, by inspection, not to write through the pointer. Scoped to
// exactly this one expression via a GNU statement-expression, so -Wcast-qual
// stays fully active for every other cast in the codebase; only intentional,
// reviewed uses of this macro are exempted.
#define QUIRE_DISCARD_CONST(ptr)                                                                                                    \
    (__extension__({                                                                                                                \
        __typeof__(ptr) QUIRE_DISCARD_CONST_tmp_ = (ptr);                                                                           \
        _Pragma("GCC diagnostic push")                                                                                              \
            _Pragma("GCC diagnostic ignored \"-Wcast-qual\"") void *QUIRE_DISCARD_CONST_result_ = (void *)QUIRE_DISCARD_CONST_tmp_; \
        _Pragma("GCC diagnostic pop")                                                                                               \
            QUIRE_DISCARD_CONST_result_;                                                                                            \
    }))

#else

#define QUIRE_PRINTF(fmtIndex, argIndex)
#define QUIRE_CONST
#define QUIRE_PURE
#define QUIRE_WARN_UNUSED_RESULT
#define QUIRE_DISCARD_CONST(ptr) ((void *)(ptr))

#endif

#endif