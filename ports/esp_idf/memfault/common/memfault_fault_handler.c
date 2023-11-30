//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Hooks

#include <stdlib.h>

#include "esp_attr.h"
#include "esp_core_dump.h"
#include "esp_err.h"
#ifdef __XTENSA__
  #include "freertos/xtensa_api.h"
  #include "memfault/panics/arch/xtensa/xtensa.h"
#elif __riscv
  #include "memfault/panics/arch/riscv/riscv.h"
  #include "riscv/rvruntime-frames.h"
#endif
#include "memfault/esp_port/version.h"
#include "memfault/panics/coredump.h"
#include "memfault/panics/fault_handling.h"

#ifndef ESP_PLATFORM
  #error "The port assumes the esp-idf is in use!"
#endif

// Header refactor and watchpoint definitions added in 4.3.0
// Only include if watchpoint stack overflow detection enabled
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0) && \
  defined(CONFIG_FREERTOS_WATCHPOINT_END_OF_STACK)
  #include "soc/soc_caps.h"
#endif  // ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0) &&
        // defined(CONFIG_FREERTOS_WATCHPOINT_END_OF_STACK)

// Note: The esp-idf implements abort which will invoke the esp-idf coredump handler as well as a
// chip reboot so we just piggyback off of that
void memfault_fault_handling_assert(void *pc, void *lr) {
  memfault_arch_fault_handling_assert(pc, lr, kMfltRebootReason_Assert);
  abort();
}

// This wrap handles the libc <assert.h> assert() calls, which we cannot
// intercept at __assert_func() as usual because that's already supplied by
// esp-idf. Note that if the assert path went through
// MEMFAULT_ASSERT()->memfault_fault_handling_assert()->abort()->__wrap_panic_abort(),
// memfault_arch_fault_handling_assert() is called twice. This isn't a problem,
// because there's logic inside memfault_reboot_tracking_mark_reset_imminent()
// to ignore the second call, and this gets us the highest-frame PC and LR in
// the assert info when MEFMAULT_ASSERT() is called.
void IRAM_ATTR __attribute__((noreturn, no_sanitize_undefined))
__real_panic_abort(const char *details);
void IRAM_ATTR __attribute__((noreturn, no_sanitize_undefined))
__wrap_panic_abort(const char *details) {
  void *pc;
  MEMFAULT_GET_PC(pc);
  void *lr;
  MEMFAULT_GET_LR(lr);
  memfault_arch_fault_handling_assert(pc, lr, kMfltRebootReason_Assert);

  __real_panic_abort(details);
}

// This header is only available for ESP-IDF >= 4.2
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 2, 0)
  #include "esp_private/panic_internal.h"
// Ensure the substituted function signature matches the original function
_Static_assert(__builtin_types_compatible_p(__typeof__(&panic_abort),
                                            __typeof__(&__wrap_panic_abort)),
               "Error: esp panic_abort wrapper is not compatible with esp-idf implementation");
#endif

void memfault_fault_handling_assert_extra(void *pc, void *lr, sMemfaultAssertInfo *extra_info) {
  memfault_arch_fault_handling_assert(pc, lr, extra_info->assert_reason);
  abort();
}

//! Invoked when a panic is detected in the esp-idf when coredumps are enabled
//!
//! @note This requires the following sdkconfig options:
//!   CONFIG_ESP32_ENABLE_COREDUMP=y
//!   CONFIG_ESP32_ENABLE_COREDUMP_TO_FLASH=y
//!
//! @note This is a drop in replacement for the pre-existing flash coredump handler.
//! The default implementation is replaced by leveraging GCCs --wrap feature
//!    https://github.com/espressif/esp-idf/blob/v4.0/components/esp32/panic.c#L620
//!
//! @note The signature for the wrapped function changed in esp-idf v4.3+, then
//! later backported to the 4.2 branch in v4.2.3. Support that change with a
//! version check (see static assert below for verifying the signature is
//! correct)
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 2, 3)
void __wrap_esp_core_dump_to_flash(panic_info_t *info) {
  #ifdef __XTENSA__
  XtExcFrame *fp = (void *)info->frame;
  #elif __riscv
  RvExcFrame *fp = (void *)info->frame;
  #endif
#else
void __wrap_esp_core_dump_to_flash(XtExcFrame *fp) {
#endif

  eMemfaultRebootReason reason;

/*
 * To better classify the exception, we need panic_info_t provided by
 * esp-idf v4.2.3+. For older versions just assume kMfltRebootReason_HardFault
 */
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 2, 3)
  switch (info->exception) {
    case PANIC_EXCEPTION_DEBUG:
      reason = kMfltRebootReason_DebuggerHalted;
      break;
    default:
      // Default to HardFault until other reasons are handled
      reason = kMfltRebootReason_HardFault;
      break;
  }
#else
  reason = kMfltRebootReason_HardFault;
#endif  // ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 2, 3)

#ifdef __XTENSA__
  // Clear "EXCM" bit so we don't have to correct PS.OWB to get a good unwind This will also be
  // more reflective of the state of the register prior to the "panicHandler" being invoked
  const uint32_t corrected_ps = fp->ps & ~(PS_EXCM_MASK);

  sMfltRegState regs = {
    .collection_type = (uint32_t)kMemfaultEsp32RegCollectionType_ActiveWindow,
    .pc = fp->pc,
    .ps = corrected_ps,
    .a =
      {
        fp->a0,
        fp->a1,
        fp->a2,
        fp->a3,
        fp->a4,
        fp->a5,
        fp->a6,
        fp->a7,
        fp->a8,
        fp->a9,
        fp->a10,
        fp->a11,
        fp->a12,
        fp->a13,
        fp->a14,
        fp->a15,
      },
    .sar = fp->sar,
  // the below registers are not available on the esp32s2; leave them zeroed
  // in the coredump
  #if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S3
    .lbeg = fp->lbeg,
    .lend = fp->lend,
    .lcount = fp->lcount,
  #endif
    .exccause = fp->exccause,
    .excvaddr = fp->excvaddr,
  };

  // If enabled, check if exception was triggered by stackoverflow type
  #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0) && \
    defined(CONFIG_FREERTOS_WATCHPOINT_END_OF_STACK)
    /*
     * See Xtensa ISA Debug Cause Register section:
     * https://www.cadence.com/content/dam/cadence-www/global/en_US/documents/tools/ip/tensilica-ip/isa-summary.pdf#_OPENTOPIC_TOC_PROCESSING_d61e48262
     */
    // Mask to select bits indicating a data breakpoint/watchpoint
    #define DBREAK_EXCEPTION_MASK (1 << 2)

    #define _WATCHPOINT_VAL_SHIFT (8)
    #define _WATCHPOINT_VAL_MASK (0xF << _WATCHPOINT_VAL_SHIFT)
    // Extracts DBNUM bits to determine watchpoint that triggered exception
    #define WATCHPOINT_VAL_GET(reg) ((reg & _WATCHPOINT_VAL_MASK) >> _WATCHPOINT_VAL_SHIFT)
    // Watchpoint to detect stack overflow
    #define END_OF_STACK_WATCHPOINT_VAL (SOC_CPU_WATCHPOINTS_NUM - 1)

  if (info->exception == PANIC_EXCEPTION_DEBUG) {
    // Read debugcause register into debug_cause local var
    int debug_cause;
    asm("rsr.debugcause %0" : "=r"(debug_cause));

    // Check that DBREAK bit is set and that stack overflow watchpoint was the source
    if ((debug_cause & DBREAK_EXCEPTION_MASK) &&
        (WATCHPOINT_VAL_GET(debug_cause) == END_OF_STACK_WATCHPOINT_VAL)) {
      reason = kMfltRebootReason_StackOverflow;
    }
  }
  #endif  // ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0) &&
          // defined(CONFIG_FREERTOS_WATCHPOINT_END_OF_STACK)
#elif __riscv
  sMfltRegState regs = {
    .mepc = fp->mepc,
    .ra = fp->ra,
    .sp = fp->sp,
    .gp = fp->gp,
    .tp = fp->tp,
    .t =
      {
        fp->t0,
        fp->t1,
        fp->t2,
        fp->t3,
        fp->t4,
        fp->t5,
        fp->t6,
      },
    .s =
      {
        fp->s0,
        fp->s1,
        fp->s2,
        fp->s3,
        fp->s4,
        fp->s5,
        fp->s6,
        fp->s7,
        fp->s8,
        fp->s9,
        fp->s10,
        fp->s11,
      },
    .a =
      {
        fp->a0,
        fp->a1,
        fp->a2,
        fp->a3,
        fp->a4,
        fp->a5,
        fp->a6,
        fp->a7,
      },
    .mstatus = fp->mstatus,
    .mtvec = fp->mtvec,
    .mcause = fp->mcause,
    .mtval = fp->mtval,
    .mhartid = fp->mhartid,
  };
#endif

  memfault_fault_handler(&regs, reason);
}

// Ensure the substituted function signature matches the original function
_Static_assert(__builtin_types_compatible_p(__typeof__(&esp_core_dump_to_flash),
                                            __typeof__(&__wrap_esp_core_dump_to_flash)),
               "Error: core dump handler is not compatible with esp-idf's default implementation");
