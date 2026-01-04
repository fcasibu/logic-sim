#ifndef BASE_H
#define BASE_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG_MODE
#define DEBUG 1
#else
#define DEBUG 0
#endif

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef i8 b8;
typedef i32 b32;
typedef i64 b64;

typedef float f32;
typedef double f64;

typedef uintptr_t uptr;
typedef size_t usize;
typedef ptrdiff_t isize;

#define internal static
#define local_persist static
#define global static
#define local_const static const
#define global_const static const

#define B(x) (x)
#define KB(x) ((x) << 10)
#define MB(x) ((x) << 20)
#define GB(x) ((x) << 30)
#define TB(x) ((x) << 40)

#define Thousand(x) ((x) * 1000)
#define Million(x) ((x) * 1000000)
#define Billion(x) ((x) * 1000000000)

#define Stmt(s) \
    do {        \
        s       \
    } while (0)

#define Enum(type, name) \
    type name;           \
    enum

#if DEBUG
#define Assert(c)                                                        \
    Stmt(if (!(c)) {                                                     \
        fprintf(stderr, "%s:%d: Assertion Error\n", __FILE__, __LINE__); \
        abort();                                                         \
    })
#else
#define Assert(c)
#endif

#define Unused(v) ((void)(v))
#define Todo(m) Stmt(fprintf(stderr, "%s:%d: TODO: %s", __FILE__, __LINE__, (m)); abort();)
#define Unreachable(m) \
    Stmt(fprintf(stderr, "%s:%d: UNREACHABLE: %s", __FILE__, __LINE__, m); abort();)

#define ArrayCount(a) ((sizeof(a)) / (sizeof(*a)))
#define ArraySafe(a, i) ((a)[(i) % ArrayCount(a)])

#define INVALID_DEFAULT_CASE              \
    default: {                            \
        Unreachable("Invalid code path"); \
    } break

#define HasFlag(fi, fl) (((fi) & (fl)) != 0)
#define HasFlags(fi, fl) (((fi) & (fl)) == (fl))
#define AddFlag(fi, fl) ((fi) |= (fl))
#define ClearFlag(fi, fl) ((fi) &= (~fl))

#define Min(a, b) ((a) > (b) ? (b) : (a))
#define Max(a, b) ((a) > (b) ? (a) : (b))

#endif // BASE_H
