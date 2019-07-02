#pragma once

//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Hooks for linking system assert infra with Memfault error collection

#include "memfault/core/compiler.h"
#include "memfault/panics/fault_handling.h"

#ifdef __cplusplus
extern "C" {
#endif

//! The 'MEMFAULT_ASSERT_RECORD' hook that should be called as part of the normal ASSERT flow in
//! the system. This will save the reboot info to persist across boots.
//!
//! NB: We may also want to think about whether we should save off something like __LINE__ (or use
//! a compiler flag) that blocks the compiler from coalescing asserts (since that could be
//! confusing to an end customer trying to figure out the exact assert hit)
#define MEMFAULT_ASSERT_RECORD(_x)                                 \
  do {                                                             \
    void *pc;                                                      \
    MEMFAULT_GET_PC(pc);                                           \
    void *lr = MEMFAULT_GET_LR();                                  \
    memfault_fault_handling_assert(pc, lr, _x);                    \
  } while (0)

#define MEMFAULT_ASSERT(exp)                    \
  do {                                          \
    if (!(exp)) {                               \
      MEMFAULT_ASSERT_RECORD(0);                \
    }                                           \
  } while (0)

#ifdef __cplusplus
}
#endif
