#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
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

// Check for ARM Cortex-A/R target architecture (v7 and v8)
#if MEMFAULT_COMPILER_ARM
  #if defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_8A__) || \
    defined(__ARM_ARCH_8R__)
    #define MEMFAULT_COMPILER_ARM_V7_A_R 1
  #else
    #define MEMFAULT_COMPILER_ARM_V7_A_R 0
  #endif

  // Pre-Cortex ARM families: ARM7TDMI (ARMv4T), ARM9 (ARMv5TE/TEJ), ARM11 (ARMv6).
  // These lack Cortex-M system registers (SCB, NVIC, etc.) so must not be treated
  // as Cortex-M even though MEMFAULT_COMPILER_ARM_V7_A_R is also 0 for them.
  #if defined(__ARM_ARCH_4T__) || defined(__ARM_ARCH_5T__) || defined(__ARM_ARCH_5TE__) || \
    defined(__ARM_ARCH_5TEJ__) || defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) ||   \
    defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) || defined(__ARM_ARCH_6ZK__) ||   \
    defined(__ARM_ARCH_6T2__)
    #define MEMFAULT_COMPILER_ARM_LEGACY 1
  #else
    #define MEMFAULT_COMPILER_ARM_LEGACY 0
  #endif

  // Prefer positive detection via __ARM_ARCH_PROFILE (GCC/Clang define this as
  // 'A', 'R', or 'M'). Fall back to negation when unavailable, with explicit
  // A/R and legacy exclusions to avoid misclassifying unknown ARM variants.
  #if defined(__ARM_ARCH_PROFILE)
    #define MEMFAULT_COMPILER_ARM_CORTEX_M (__ARM_ARCH_PROFILE == 'M')
  #else
    #define MEMFAULT_COMPILER_ARM_CORTEX_M \
      (!MEMFAULT_COMPILER_ARM_V7_A_R && !MEMFAULT_COMPILER_ARM_LEGACY)
  #endif
#else
  #define MEMFAULT_COMPILER_ARM_V7_A_R 0
  #define MEMFAULT_COMPILER_ARM_LEGACY 0
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
