#ifndef _HOBBYSCRIPT_COMMON_H
#define _HOBBYSCRIPT_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// #define DEBUG_TRACE_EXECUTION
// #define DEBUG_PRINT_CODE

// #define DEBUG_STRESS_GC
// #define DEBUG_LOG_GC

#define NAN_BOXING

#define UNUSED __attribute__((unused))
#define FALLTHROUGH __attribute__((fallthrough))

#define U8_COUNT (UINT8_MAX + 1)

typedef uint8_t  u8;
typedef int8_t   s8;
typedef uint16_t u16;
typedef int16_t  s16;
typedef uint32_t u32;
typedef int32_t  s32;
typedef uint64_t u64;
typedef int64_t  s64;

typedef float    f32;
typedef double   f64;

#endif // _HOBBYSCRIPT_COMMON_H
