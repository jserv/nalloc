#ifndef INTERNAL_UTIL_H
#define INTERNAL_UTIL_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#undef assert
#ifdef NDEBUG
#define assert(expr) ((void)0)
#else
#define assert(expr) (likely(expr) ? (void)0 : abort())
#endif

/* clang */
#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

#if defined(__GNUC__) || __has_builtin(__builtin_expect)
#define likely(x) __builtin_expect (!!(x), 1)
#define unlikely(x) __builtin_expect (!!(x), 0)
#define UNUSED __attribute__((unused))
#define EXPORT __attribute__((visibility("default")))
#define COLD __attribute__((cold))
#else
#define likely(x) (x)
#define unlikely(x) (x)
#define UNUSED
#define EXPORT
#define COLD
#endif

#endif /* INTERNAL_UTILS_H */
