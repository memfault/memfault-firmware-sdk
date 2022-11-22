//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Hooks


#include <stdlib.h>

#include "esp_core_dump.h"
#ifdef __XTENSA__
#include "xtensa/xtensa_api.h"
#include "xt_utils.h"
#include "memfault/panics/arch/xtensa/xtensa.h"
#elif __riscv
#include "riscv/rvruntime-frames.h"
#include "memfault/panics/arch/riscv/riscv.h"
#endif
#include "memfault/esp_port/version.h"
#include "memfault/panics/coredump.h"
#include "memfault/panics/fault_handling.h"

#ifndef ESP_PLATFORM
#  error "The port assumes the esp-idf is in use!"
#endif

// Note: The esp-idf implements abort which will invoke the esp-idf coredump handler as well as a
// chip reboot so we just piggback off of that
void memfault_fault_handling_assert(void *pc, void *lr) { abort(); }

void memfault_fault_handling_assert_extra(void *pc, void *lr, sMemfaultAssertInfo *extra_info) {
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
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4,2,3)
void __wrap_esp_core_dump_to_flash(panic_info_t *info) {
#ifdef __XTENSA__
  XtExcFrame *fp = (void *)info->frame;
#elif __riscv
  RvExcFrame *fp = (void *)info->frame;
#endif
#else
void __wrap_esp_core_dump_to_flash(XtExcFrame *fp) {
#endif

#ifdef __XTENSA__
  // Clear "EXCM" bit so we don't have to correct PS.OWB to get a good unwind This will also be
  // more reflective of the state of the register prior to the "panicHandler" being invoked
  const uint32_t corrected_ps = fp->ps & ~(PS_EXCM_MASK);

  sMfltRegState regs = {
    .collection_type = (uint32_t)kMemfaultEsp32RegCollectionType_ActiveWindow,
    .pc = fp->pc,
    .ps = corrected_ps,
    .a = {
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
#if CONFIG_IDF_TARGET_ESP32
    .lbeg = fp->lbeg,
    .lend = fp->lend,
    .lcount = fp->lcount,
#elif CONFIG_IDF_TARGET_ESP32S2
    // TODO implement fault capture for the ESP32-S2
#elif CONFIG_IDF_TARGET_ESP32S3
    // TODO implement fault capture for the ESP32-S3
#endif
    .exccause = fp->exccause,
    .excvaddr = fp->excvaddr,
  };
#elif __riscv
  sMfltRegState regs = {
    .collection_type = (uint32_t)kMemfaultEsp32RegCollectionType_ActiveWindow,
    .mepc = fp->mepc,
    .ra = fp->ra,
    .sp = fp->sp,
    .gp = fp->gp,
    .tp = fp->tp,
    .t = {
      fp->t0,
      fp->t1,
      fp->t2,
      fp->t3,
      fp->t4,
      fp->t5,
      fp->t6,
    },
    .s = {
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
    .a = {
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

  memfault_fault_handler(&regs, kMfltRebootReason_HardFault);
}

// Ensure the substituted function signature matches the original function
_Static_assert(__builtin_types_compatible_p(__typeof__(&esp_core_dump_to_flash),
                                            __typeof__(&__wrap_esp_core_dump_to_flash)),
               "Error: core dump handler is not compatible with esp-idf's default implementation");
