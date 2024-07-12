//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Hooks the Memfault log collection to the Zephyr LOG_MODE_MINIMAL logging
//! system.

// clang-format off
#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)
#include MEMFAULT_ZEPHYR_INCLUDE(sys/atomic.h)
#include MEMFAULT_ZEPHYR_INCLUDE(logging/log.h)

#include <stdarg.h>

#include "memfault/components.h"
// clang-format on

enum {
  MFLT_LOG_BUFFER_BUSY,
};
static atomic_t s_log_output_busy_flag;

static void prv_log_init(void) {
  // Static RAM storage where logs will be stored.
  static uint8_t s_mflt_log_buf_storage[CONFIG_MEMFAULT_LOGGING_RAM_SIZE];
  memfault_log_boot(s_mflt_log_buf_storage, sizeof(s_mflt_log_buf_storage));
}

static void prv_memfault_log_vprintf(const char *fmt, va_list args) {
  // Force synchronization at this level.
  if (atomic_test_and_set_bit(&s_log_output_busy_flag, MFLT_LOG_BUFFER_BUSY)) {
    return;
  }

  // This is idempotent; but sensitive to concurrent access. The atomic flag
  // that protects this function should be good enough, since it only has to
  // succeed once. The vulnerability is only if a user is accessing
  // memfault_log_save_preformatted_nolock() directly (should not be done).
  prv_log_init();

  // Render the log message into a buffer. Static storage class, so it doesn't
  // contribute to stack usage which could be problematic in this call site.
  static uint8_t s_zephyr_render_buf[128];
  int save_length = vsnprintk(s_zephyr_render_buf, sizeof(s_zephyr_render_buf), fmt, args);
  if (save_length > 0) {
    // Always set to info, since we can't reliably determine the log level
    memfault_log_save_preformatted_nolock(kMemfaultPlatformLogLevel_Info, s_zephyr_render_buf,
                                          save_length);
  }

  atomic_clear_bit(&s_log_output_busy_flag, MFLT_LOG_BUFFER_BUSY);
}

void __wrap_z_log_minimal_printk(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  prv_memfault_log_vprintf(fmt, args);

  va_start(args, fmt);

  // Now call zephyr printk
  z_log_minimal_vprintk(fmt, args);

  va_end(args);
}

// Ensure the substituted function signature matches the original function
_Static_assert(__builtin_types_compatible_p(__typeof__(&z_log_minimal_printk),
                                            __typeof__(&__wrap_z_log_minimal_printk)),
               "Error: check z_log_minimal_printk function signature");
