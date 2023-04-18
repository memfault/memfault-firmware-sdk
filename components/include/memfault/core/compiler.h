#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Wrappers for common macros & compiler specifics

#ifdef __cplusplus
extern "C" {
#endif

#define MEMFAULT_QUOTE(str) #str
#define MEMFAULT_EXPAND_AND_QUOTE(str) MEMFAULT_QUOTE(str)
#define MEMFAULT_CONCAT_(x, y) x##y
#define MEMFAULT_CONCAT(x, y) MEMFAULT_CONCAT_(x, y)

// Given a static string definition, compute the strlen equivalent
// (i.e MEMFAULT_STATIC_STRLEN("abcd") == 4)
#define MEMFAULT_STATIC_STRLEN(s) (sizeof(s) - 1)

// Convenience macros for distinguishing ARM variants
#if defined(__arm__) || defined(__ICCARM__) || defined(__TI_ARM__)
  #define MEMFAULT_COMPILER_ARM 1
#else
  #define MEMFAULT_COMPILER_ARM 0
#endif

// Check for ARMv7-A/R target architecture
#if MEMFAULT_COMPILER_ARM
  #if defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7A__)
    #define MEMFAULT_COMPILER_ARM_V7_A_R 1
  #else
    #define MEMFAULT_COMPILER_ARM_V7_A_R 0
  #endif

  // If not Cortex-A/R, select Cortex-M family
  #define MEMFAULT_COMPILER_ARM_CORTEX_M !MEMFAULT_COMPILER_ARM_V7_A_R
#else
  #define MEMFAULT_COMPILER_ARM_V7_A_R 0
  #define MEMFAULT_COMPILER_ARM_CORTEX_M 0
#endif

//
// Pick up compiler specific macro definitions
//

#if defined(__CC_ARM)
  #include "compiler_armcc.h"
#elif defined(__TI_ARM__)
  #include "compiler_ti_arm.h"
#elif defined(__GNUC__) || defined(__clang__)
  #include "compiler_gcc.h"
#elif defined(__ICCARM__)
  #include "compiler_iar.h"
#else
  #error "New compiler to add support for!"
#endif

#ifdef __cplusplus
}
#endif
