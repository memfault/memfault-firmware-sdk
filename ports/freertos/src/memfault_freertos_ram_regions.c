//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @note This file enables collecting a portion of all the FreeRTOS task state in a minimal RAM
//! footprint. If you are able to collect all of RAM in your coredump, there is no need to use the
//! utilities in this file
//!
//! To utilize this implementation to capture a portion of all the FreeRTOS tasks:
//!
//! 1) Update linker script to place FreeRTOS at a fixed address:
//!
//!    .bss (NOLOAD) :
//!    {
//!        _sbss = . ;
//!        __bss_start__ = _sbss;
//!        __memfault_capture_bss_start = .;
//!        /* Place all objects from the FreeRTOS timers and tasks modules here.
//!           Note that some build systems will use 'timers.o' as the object
//!           file name, and some may use variations of 'timers.c.o' or
//!           'timers.obj' etc. This pattern should capture all of them. */
//!         *tasks*.o*(.bss COMMON .bss*)
//!         *timers*.o*(.bss COMMON .bss*)
//!        __memfault_capture_bss_end = .;
//!
//! 2) Add this file to your build and update FreeRTOSConfig.h to
//!    include "memfault/ports/freertos_trace.h"
//!
//! 3) Implement memfault_platform_sanitize_address_range(). This routine is used
//!    to run an extra sanity check in case any task context state is corrupted, i.e
//!
//!     #include "memfault/ports/freertos_coredump.h"
//!     #include "memfault/core/math.h"
//!     // ...
//!     size_t memfault_platform_sanitize_address_range(void *start_addr, size_t desired_size) {
//!       // Note: This will differ depending on the memory map of the MCU
//!       const uint32_t ram_start = 0x20000000;
//!       const uint32_t ram_end = 0x20000000 + 64 * 1024;
//!       if ((uint32_t)start_addr >= ram_start && (uint32_t)start_addr < ram_end) {
//!         return MEMFAULT_MIN(desired_size, ram_end - (uint32_t)start_addr);
//!       }
//!
//!      // invalid address
//!      return 0;
//!     }
//!
//! 4) Update memfault_platform_coredump_get_regions() to include FreeRTOS state
//!    and new regions:
//!
//!   const sMfltCoredumpRegion *memfault_platform_coredump_get_regions(
//!       const sCoredumpCrashInfo *crash_info, size_t *num_regions) {
//!     int region_idx = 0;
//!
//!     const size_t active_stack_size_to_collect = 512;
//!
//!     // first, capture the active stack
//!     s_coredump_regions[0] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(
//!        crash_info->stack_address,
//!        memfault_platform_sanitize_address_range(crash_info->stack_address,
//!          active_stack_size_to_collect));
//!     region_idx++;
//!
//!     extern uint32_t __memfault_capture_bss_start;
//!     extern uint32_t __memfault_capture_bss_end;
//!     const size_t memfault_region_size = (uint32_t)&__memfault_capture_bss_end -
//!         (uint32_t)&__memfault_capture_bss_start;
//!
//!     s_coredump_regions[region_idx] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(
//!        &__memfault_capture_bss_start, memfault_region_size);
//!     region_idx++;
//!
//!     region_idx += memfault_freertos_get_task_regions(&s_coredump_regions[region_idx],
//!         MEMFAULT_ARRAY_SIZE(s_coredump_regions) - region_idx);
//!
//!     *num_regions = region_idx;
//!     return &s_coredump_regions[0];
//!   }

#include "memfault/ports/freertos_coredump.h"

#include <stdint.h>

#include "memfault/config.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/math.h"
#include "memfault/panics/coredump.h"

// Espressif's esp-idf project uses a different include directory by default.
#if defined(ESP_PLATFORM)
  #include "sdkconfig.h"
  #if !defined(CONFIG_IDF_TARGET_ESP8266)
    #define MEMFAULT_USE_ESP32_FREERTOS_INCLUDE
  #endif
#endif

#if defined(MEMFAULT_USE_ESP32_FREERTOS_INCLUDE)
  #include "freertos/FreeRTOS.h"
  #include "freertos/task.h"
#else  // MEMFAULT_USE_ESP32_FREERTOS_INCLUDE
  #include "FreeRTOS.h"
  #include "task.h"
#endif  // MEMFAULT_USE_ESP32_FREERTOS_INCLUDE

#if !defined(MEMFAULT_FREERTOS_TRACE_ENABLED)
#error "'#include "memfault/ports/freertos_trace.h"' must be added to FreeRTOSConfig.h"
#endif

// This feature is opt-in, since it can fail if the TCB data structures are
// corrupt, and results in a lost coredump.
#if !defined(MEMFAULT_COREDUMP_COMPUTE_THREAD_STACK_USAGE)
  #define MEMFAULT_COREDUMP_COMPUTE_THREAD_STACK_USAGE 0
#endif

#if MEMFAULT_COREDUMP_COMPUTE_THREAD_STACK_USAGE && !INCLUDE_uxTaskGetStackHighWaterMark
  #warning \
    MEMFAULT_COREDUMP_COMPUTE_THREAD_STACK_USAGE requires uxTaskGetStackHighWaterMark() API.\
    Add '#define INCLUDE_uxTaskGetStackHighWaterMark 1' to FreeRTOSConfig.h to enable it,\
    or set '#define MEMFAULT_COREDUMP_COMPUTE_THREAD_STACK_USAGE 0' in\
    memfault_platform_config.h to disable this warning.
  #undef MEMFAULT_COREDUMP_COMPUTE_THREAD_STACK_USAGE
  #define MEMFAULT_COREDUMP_COMPUTE_THREAD_STACK_USAGE 0
#endif

#if !defined(MEMFAULT_FREERTOS_WARN_STACK_HIGH_ADDRESS_UNAVAILABLE)
  #if !configRECORD_STACK_HIGH_ADDRESS
    #warning \
      To see FreeRTOS thread stack sizes in coredumps, Add\
      '#define configRECORD_STACK_HIGH_ADDRESS 1' to FreeRTOSConfig.h. Otherwise, set\
      '#define MEMFAULT_FREERTOS_WARN_STACK_HIGH_ADDRESS_UNAVAILABLE 0' in\
      memfault_platform_config.h to disable this warning.
  #endif
#endif

#if MEMFAULT_COREDUMP_COMPUTE_THREAD_STACK_USAGE
static struct MfltTaskWatermarks {
  uint32_t high_watermark;
} s_memfault_task_watermarks[MEMFAULT_PLATFORM_MAX_TRACKED_TASKS];
#endif

// If the MEMFAULT_PLATFORM_FREERTOS_TCB_SIZE value is set to 0, apply a default
// to MEMFAULT_FREERTOS_TCB_SIZE here. This value can be overriden by setting
// MEMFAULT_PLATFORM_FREERTOS_TCB_SIZE in memfault_platform_config.h. See
// include/memfault/default_config.h for more information.
#if MEMFAULT_PLATFORM_FREERTOS_TCB_SIZE
  #define MEMFAULT_FREERTOS_TCB_SIZE MEMFAULT_PLATFORM_FREERTOS_TCB_SIZE
#else
  #if tskKERNEL_VERSION_MAJOR <= 8
    //! Note: What we want here is sizeof(TCB_t) but that is a private declaration in FreeRTOS
    //! tasks.c. Since the static declaration doesn't exist for FreeRTOS kernel <= 8, fallback to a
    //! generous size that will include the entire TCB. A user of the SDK can tune the size by
    //! adding MEMFAULT_FREERTOS_TCB_SIZE to memfault_platform_config.h
    #define MEMFAULT_FREERTOS_TCB_SIZE 200
  #else
    #define MEMFAULT_FREERTOS_TCB_SIZE sizeof(StaticTask_t)
  #endif
#endif

//! Allow overriding the MEMFAULT_LOG_ERROR() call when the task registry is
//! full and a new task is created. Some systems are unable to issue logs from
//! the task creation calling context (i.e the task is created before the
//! logging system is initialized).
//!
//! For example, for ESP-IDF, it might be appropriate to instead use:
//! #define MEMFAULT_FREERTOS_REGISTRY_FULL_ERROR_LOG(...) ESP_EARLY_LOGE("mflt", __VA_ARGS__)
//!
//! The overriding is done in a file included from here, in case additional
//! headers are needed
#if defined(MEMFAULT_FREERTOS_REGISTRY_FULL_ERROR_LOG_INCLUDE)
  #include MEMFAULT_FREERTOS_REGISTRY_FULL_ERROR_LOG_INCLUDE
#else
  #define MEMFAULT_FREERTOS_REGISTRY_FULL_ERROR_LOG MEMFAULT_LOG_ERROR
#endif

//! This array holds the tracked TCB references
static void *s_task_tcbs[MEMFAULT_PLATFORM_MAX_TRACKED_TASKS];
#define EMPTY_SLOT 0

static bool prv_find_slot(size_t *idx, void *desired_tcb) {
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(s_task_tcbs); i++) {
    if (s_task_tcbs[i] == desired_tcb) {
      *idx = i;
      return true;
    }
  }

  return false;
}

// We're not locking around the 'memfault_freertos_trace_task_create()' /
// 'memfault_freertos_trace_task_delete()' operations, since they are expected
// to be called as part of the FreeRTOS kernel trace hooks ('traceTASK_CREATE()'
// / 'traceTASK_DELETE()'), which are already serialized with a kernel lock or
// port critical section
void memfault_freertos_trace_task_create(void *tcb) {
  size_t idx = 0;

  const bool slot_found = prv_find_slot(&idx, EMPTY_SLOT);
  if (slot_found) {
    s_task_tcbs[idx] = tcb;
  }

  if (!slot_found) {
    MEMFAULT_FREERTOS_REGISTRY_FULL_ERROR_LOG(
      "Task registry full (" MEMFAULT_EXPAND_AND_QUOTE(MEMFAULT_PLATFORM_MAX_TRACKED_TASKS) ")");
  }
}

void memfault_freertos_trace_task_delete(void *tcb) {
  size_t idx = 0;
  if (!prv_find_slot(&idx, tcb)) {
    // A TCB not currently in the registry
    return;
  }

  s_task_tcbs[idx] = EMPTY_SLOT;
}

#if MEMFAULT_COREDUMP_COMPUTE_THREAD_STACK_USAGE
static uint32_t prv_stack_bytes_high_watermark(void *tcb_address) {
  // Note: ideally we would sanitize the pxTCB->pxStack value here (which is
  // used when computing high watermark) to prevent a memory error. We could
  // potentially scan in decrementing addresses from pxTopOfStack (full
  // descending stack), looking for the first n run of 0xa5 bytes, and use that
  // as the presumed "high watermark". We could then sanitize that access, so
  // we'd be less likely to trip a memory error. For now, just rely on the
  // earlier sanity check on pxTopOfStack being a valid address, and assume
  // pxStack hasn't been corrupted in that case.

  // Note: uxTaskGetStackHighWaterMark is very simple, and just scans the
  // stack for the first non 0xa5 byte. It's safe to call it from the fault
  // handler context (no locks, no memory allocation, etc)

  // The value returned here is the high watermark, *NOT* the "bytes unused".
  // Memfault's backend will convert it using the TCB_t stack size. We cannot
  // access the TCB_t definition from this compilation unit as it is
  // intentionally opaque in FreeRTOS, unfortunately.
  return uxTaskGetStackHighWaterMark((TaskHandle_t)tcb_address);
}
#endif

size_t memfault_freertos_get_task_regions(sMfltCoredumpRegion *regions, size_t num_regions) {
  if (regions == NULL || num_regions == 0) {
    return 0;
  }

  size_t region_idx = 0;

  // First we will try to store all the task TCBs. This way if we run out of space
  // while storing tasks we will still be able to recover the state of all the threads
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(s_task_tcbs) && region_idx < num_regions; i++) {
    void *tcb_address = s_task_tcbs[i];
    if (tcb_address == EMPTY_SLOT) {
      continue;
    }

    const size_t tcb_size = memfault_platform_sanitize_address_range(
        tcb_address, MEMFAULT_FREERTOS_TCB_SIZE);
    if (tcb_size == 0) {
      // An invalid address, scrub the TCB from the list so we don't try to dereference
      // it when grabbing stacks below and move on.
      s_task_tcbs[i] = EMPTY_SLOT;
      continue;
    }

    regions[region_idx] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(tcb_address, tcb_size);
    region_idx++;
  }

  // Now we store the region of the stack where context is saved. This way
  // we can unwind the stacks for threads that are not actively running
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(s_task_tcbs) && region_idx < num_regions; i++) {
    void *tcb_address = s_task_tcbs[i];
    if (tcb_address == EMPTY_SLOT) {
      continue;
    }

    // pxTopOfStack is always the first entry in the FreeRTOS TCB
    void *top_of_stack = (void *)(*(uintptr_t *)tcb_address);
    const size_t stack_size = memfault_platform_sanitize_address_range(
        top_of_stack, MEMFAULT_PLATFORM_TASK_STACK_SIZE_TO_COLLECT);
    if (stack_size == 0) {
      continue;
    }

#if MEMFAULT_COREDUMP_COMPUTE_THREAD_STACK_USAGE
    s_memfault_task_watermarks[i].high_watermark = prv_stack_bytes_high_watermark(tcb_address);
#endif

    regions[region_idx] =  MEMFAULT_COREDUMP_MEMORY_REGION_INIT(top_of_stack, stack_size);
    region_idx++;
    if (region_idx == num_regions) {
      return region_idx;
    }
  }

#if MEMFAULT_COREDUMP_COMPUTE_THREAD_STACK_USAGE
  // Store the task TCBs and watermarks, if there's free regions
  if (region_idx < num_regions) {
    regions[region_idx] =
      MEMFAULT_COREDUMP_MEMORY_REGION_INIT(&s_task_tcbs[0], sizeof(s_task_tcbs));
    region_idx++;

    regions[region_idx] =
      MEMFAULT_COREDUMP_MEMORY_REGION_INIT(&s_memfault_task_watermarks[0], sizeof(s_memfault_task_watermarks));
    region_idx++;
  }
#endif

  return region_idx;
}
