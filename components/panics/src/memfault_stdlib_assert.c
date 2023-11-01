//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Implements a hook into C stdlib assert() to capture coredumps

#include <assert.h>
#include <stdlib.h>

#include "memfault/panics/assert.h"

// TI ARM compiler's stdlib assert is not supported right now.
#if MEMFAULT_ASSERT_CSTDLIB_HOOK_ENABLED && !defined(__TI_ARM__)

//! The symbol name for the low-level assert handler is different between Newlib
//! and ARM/IAR
  #if (defined(__CC_ARM)) || (defined(__ARMCC_VERSION)) || (defined(__ICCARM__))
void __aeabi_assert(const char *failedExpr, const char *file, int line) {
  (void)failedExpr, (void)file, (void)line;
  #elif (defined(__GNUC__))
void __assert_func(const char *file, int line, const char *func, const char *failedexpr) {
  (void)file, (void)line, (void)func, (void)failedexpr;
  #endif
  MEMFAULT_ASSERT(0);
}

#endif  // MEMFAULT_ASSERT_CSTDLIB_HOOK_ENABLED && !defined(__TI_ARM__)
