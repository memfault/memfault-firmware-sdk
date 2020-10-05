//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Hooks for collecting coredumps from failure paths in FreeRTOS kernel

#include "memfault/panics/assert.h"

#include "FreeRTOS.h"
#include "task.h"

//! Collect a coredump when a FreeRTOS assert takes place
//!
//! Implementation for assert function present in FreeRTOSConfig.h:
//!  #define configASSERT(x) if ((x) == 0) vAssertCalled( __FILE__, __LINE__ )
void vAssertCalled(const char *file, int line) {
  MEMFAULT_ASSERT(0);
}


//! Collects a coredump when a stack overflow takes place in FreeRTOS kernel
//!
//! @note: Depends on FreeRTOS kernel being compiled with
//!  configCHECK_FOR_STACK_OVERFLOW != 0
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
  MEMFAULT_ASSERT(0);
}
