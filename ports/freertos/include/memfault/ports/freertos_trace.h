#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! This file needs to be included from your platforms FreeRTOSConfig.h to take advantage of
//! Memfault's hooks into the FreeRTOS tracing utilities

// FreeRTOSConfig.h is often included in assembly files so wrap function declarations for
// convenience to prevent compilation errors
#if !defined(__ASSEMBLER__) && !defined(__IAR_SYSTEMS_ASM__)

void memfault_freertos_trace_task_create(void *tcb);
void memfault_freertos_trace_task_delete(void *tcb);

#endif

//
// We ifndef the trace macros so it's possible for an end user to use a custom definition that
// calls Memfault's implementation as well as their own
//

#ifndef traceTASK_CREATE
#define traceTASK_CREATE(pxNewTcb) memfault_freertos_trace_task_create(pxNewTcb)
#endif

#ifndef traceTASK_DELETE
#define traceTASK_DELETE(pxTaskToDelete) memfault_freertos_trace_task_delete(pxTaskToDelete)
#endif

//! A define that is used to assert that this file has been included from FreeRTOSConfig.h
#define MEMFAULT_FREERTOS_TRACE_ENABLED 1
