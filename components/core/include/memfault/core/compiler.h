#pragma once

//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Wrappers for common macros & compiler specifics

#ifdef __cplusplus
extern "C" {
#endif

#define MEMFAULT_QUOTE(str) #str
#define MEMFAULT_EXPAND_AND_QUOTE(str) MEMFAULT_QUOTE(str)

#define MEMFAULT_PACKED __attribute__((packed))
#define MEMFAULT_NORETURN __attribute__((noreturn))
#define MEMFAULT_NAKED_FUNC __attribute__((naked))
#define MEMFAULT_UNREACHABLE __builtin_unreachable()
#define MEMFAULT_NO_OPT __attribute__((optimize("O0")))
#define MEMFAULT_ALIGNED(x) __attribute__((aligned(x)))
#define MEMFAULT_UNUSED __attribute__((unused))
#define MEMFAULT_USED __attribute__((used))
#define MEMFAULT_WEAK __attribute__((weak))
#define MEMFAULT_FUNC_ALIAS(new_func_decl, old_name) \
  new_func_decl __attribute__((alias (MEMFAULT_EXPAND_AND_QUOTE(old_name))))

#if defined(__GNUC__) && defined(__arm__)
#  define MEMFAULT_GET_LR() __builtin_return_address(0)
#  define MEMFAULT_GET_PC(_a) __asm volatile ("mov %0, pc" : "=r" (_a))
#elif defined(__GNUC__) && defined(__XTENSA__)
#  define MEMFAULT_GET_LR() __builtin_return_address(0)
#  define MEMFAULT_GET_PC(_a) _a = ({ __label__ _l; _l: &&_l;});
#elif defined(MEMFAULT_UNITTEST) || defined(__APPLE__)  // Memfault iOS SDK also #includes this header
#  define MEMFAULT_GET_LR() 0
#  define MEMFAULT_GET_PC(_a) _a = 0
#else
#  error "New architecture to add support for!"
#endif /* defined(__GNUC__) && defined(__arm__) */


#if defined(__GNUC__) || defined(__clang__)
#define MEMFAULT_PRINTF_LIKE_FUNC(a, b) __attribute__ ((format (printf, a, b)))
#endif

#if defined(MEMFAULT_UNITTEST)
// Unit tests run on native desktop and don't accept special section placements
#  define MEMFAULT_PUT_IN_SECTION(x)
#else
#  define MEMFAULT_PUT_IN_SECTION(x) __attribute__((section(x)));
#endif

#if defined(__cplusplus)
#  define MEMFAULT_STATIC_ASSERT(COND, MSG) static_assert(COND, MSG)
#else
#  define MEMFAULT_STATIC_ASSERT(COND, MSG) _Static_assert(COND, MSG)
#endif

#ifdef __cplusplus
}
#endif
