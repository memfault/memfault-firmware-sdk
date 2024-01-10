//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Hooks the Memfault logging API to zephyr's latest (V2) logging system.
//! As of Zephyr 3.2, it's the only logging system that can be used.

// clang-format off
#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)
#include MEMFAULT_ZEPHYR_INCLUDE(sys/atomic.h)
#include MEMFAULT_ZEPHYR_INCLUDE(logging/log.h)
#include MEMFAULT_ZEPHYR_INCLUDE(logging/log_backend.h)
#include MEMFAULT_ZEPHYR_INCLUDE(logging/log_output.h)
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "memfault/components.h"
#include "memfault/ports/zephyr/log_backend.h"
#include "memfault/ports/zephyr/version.h"
// clang-format on

// Deal with CONFIG_LOG_IMMEDIATE getting renamed to CONFIG_LOG_MODE_IMMEDIATE in v3.0.0 release:
//  https://github.com/zephyrproject-rtos/zephyr/commit/262cc55609b73ea61b5f999c6c6daaba20bc5240
#if defined(CONFIG_LOG_IMMEDIATE) && !defined(CONFIG_LOG_MODE_IMMEDIATE)
#define CONFIG_LOG_MODE_IMMEDIATE CONFIG_LOG_IMMEDIATE
#endif

// Can't be zero but should be reasonably sized. See ports/zephyr/Kconfig to change this size.
BUILD_ASSERT(CONFIG_MEMFAULT_LOGGING_RAM_SIZE);

// Immediate mode requires reentrancy support in the logging backend. It's very
// low overhead, so to keep things simple enable the protection in deferred mode
// too.
enum {
  MFLT_LOG_BUFFER_BUSY,
};
static atomic_t s_log_output_busy_flag;

// Define a log_output_mflt_control_block and log_output_mflt that references
// log_output_mflt_control_block via pointer. First arg to LOG_OUTPUT_DEFINE()
// is a token that the C preprocessor uses to create symbol names,
// e.g. struct log_output n and struct log_output_control_block n_control_block.
// Within the log_output_control_block there is a void *ctx we can use to store
// info we need when called by Zephyr by calling log_output_ctx_set().

static int prv_log_out(uint8_t *data, size_t length, void *ctx);
static uint8_t s_zephyr_render_buf[128];
LOG_OUTPUT_DEFINE(s_log_output_mflt, prv_log_out, s_zephyr_render_buf, sizeof(s_zephyr_render_buf));
// store the log backend ID, so we can disable it during the panic handler
const struct log_backend * s_memfault_log_backend;

// Zephyr will call our init function so we can establish some storage.
static void prv_log_init(const struct log_backend *const backend) {
  // static RAM storage where logs will be stored. Storage can be any size
  // you want but you will want it to be able to hold at least a couple logs.
  static uint8_t s_mflt_log_buf_storage[CONFIG_MEMFAULT_LOGGING_RAM_SIZE];
  memfault_log_boot(s_mflt_log_buf_storage, sizeof(s_mflt_log_buf_storage));

  // stash the log backend handle so we can disable it during the panic handler
  s_memfault_log_backend = backend;
}

// Intentionally a no-op. In immediate mode, we can't safely re-enter
// prv_log_out() to flush any buffered log data, and in deferred mode, the
// full prv_log_process() function is called, which has reentrancy protection.
// See here:
// https://github.com/zephyrproject-rtos/zephyr/blob/v3.5.0/subsys/logging/log_core.c#L393-L403
static void prv_log_panic(struct log_backend const *const backend) {}

void memfault_zephyr_log_backend_disable(void) {
  log_backend_deactivate(s_memfault_log_backend);
}

static eMemfaultPlatformLogLevel prv_map_zephyr_level_to_memfault(uint32_t zephyr_level) {
  //     Map             From            To
  return zephyr_level == LOG_LEVEL_ERR ? kMemfaultPlatformLogLevel_Error
       : zephyr_level == LOG_LEVEL_WRN ? kMemfaultPlatformLogLevel_Warning
       : zephyr_level == LOG_LEVEL_INF ? kMemfaultPlatformLogLevel_Info
       :              /* LOG_LEVEL_DBG */kMemfaultPlatformLogLevel_Debug;
}

typedef struct MfltLogProcessCtx {
  eMemfaultPlatformLogLevel memfault_level;

#if CONFIG_LOG_MODE_IMMEDIATE
  int write_idx;
#endif

} sMfltLogProcessCtx;

// LOG2 was added in Zephyr 2.6:
// https://github.com/zephyrproject-rtos/zephyr/commit/f1bb20f6b43b8b241e45f3f132f0e7bbfc65401b
// LOG2 was moved to LOG in Zephyr 3.2
// https://github.com/zephyrproject-rtos/zephyr/issues/46500
#if !MEMFAULT_ZEPHYR_VERSION_GT(3, 1)
#define log_msg_generic log_msg2_generic
#define log_output_msg_process log_output_msg2_process
#define log_msg_get_level log_msg2_get_level
#endif

static void prv_log_process(const struct log_backend *const backend,
                            union log_msg_generic *msg) {
  // This can be called in IMMEDIATE mode from an ISR, so in that case,
  // immediately bail. We currently can't safely serialize to the Memfault
  // buffer from ISR context
#if CONFIG_LOG_MODE_IMMEDIATE
  if (memfault_arch_is_inside_isr()) {
    return;
  }
#endif

  // Copied flagging from Zephry's ring buffer (rb) implementation.
  const uint32_t flags = IS_ENABLED(CONFIG_LOG_BACKEND_FORMAT_TIMESTAMP)
                       ? LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP | LOG_OUTPUT_FLAG_LEVEL
                       : LOG_OUTPUT_FLAG_LEVEL;

  // Note: This is going to trigger calls to our prv_log_out() handler

  const uint32_t zephyr_level = log_msg_get_level(&msg->log);

  // This only needs to stay in scope for this function since log_output_msg_process()
  // calls prv_log_out() directly which is the only place we access the context
  sMfltLogProcessCtx log_process_ctx = {
    .memfault_level = prv_map_zephyr_level_to_memfault(zephyr_level),
  };

  if (atomic_test_and_set_bit(&s_log_output_busy_flag, MFLT_LOG_BUFFER_BUSY)) {
    return;
  }

  log_output_ctx_set(&s_log_output_mflt, &log_process_ctx);
  log_output_msg_process(&s_log_output_mflt, &msg->log, flags);

  atomic_clear_bit(&s_log_output_busy_flag, MFLT_LOG_BUFFER_BUSY);
}

static void prv_log_dropped(const struct log_backend *const backend, uint32_t cnt) {
  ARG_UNUSED(backend);
  log_output_dropped_process(&s_log_output_mflt, cnt);
}

const struct log_backend_api log_backend_mflt_api = {
  .process          = prv_log_process,
  .panic            = prv_log_panic,
  .init             = prv_log_init,
  // dropped handler is only used in deferred mode, so save some space by not
  // including it when immediate mode is enabled.
  .dropped          = IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ? NULL : prv_log_dropped,
};

// Define a couple of structs needed by the logging backend infrastructure.
// Binds our log_backend_mflt_api into the logger.
LOG_BACKEND_DEFINE(log_backend_mflt, log_backend_mflt_api, true);

// Tie Memfault's log function to the Zephyr buffer sender. This is *the* connection to Memfault.
static int prv_log_out(uint8_t *data, size_t length, void *ctx) {
#if defined(CONFIG_LOG_MODE_IMMEDIATE)
  // In synchronous mode, logging can occur from ISRs. The zephyr fault handlers are chatty so
  // don't save info while in an ISR to avoid wrapping over the info we are collecting.
  // This function may also be run from LOG_PANIC. We also want to skip saving data in this case
  // because the context object uses local stack memory which may not be valid when run from
  // LOG_PANIC
  if (memfault_arch_is_inside_isr()) {
    return (int)length;
  }
#endif

  sMfltLogProcessCtx *mflt_ctx = (sMfltLogProcessCtx *)ctx;
  size_t save_length = length;

#if CONFIG_LOG_MODE_IMMEDIATE
  // A check to help us catch if behavior changes in a future release of Zephyr
  MEMFAULT_SDK_ASSERT(length <= 1);

  // A few notes about immediate mode:
  //
  //  1. We are going to re-purpose "s_zephyr_render_buf" as it is never actually used by the
  //     Zephyr logging stack in immediate mode and we don't want to waste extra ram:
  //       https://github.com/zephyrproject-rtos/zephyr/blob/15fdee04e3daf4d63064e4195aeeef6ccc52e694/subsys/logging/log_output.c#L105-L112
  //
  //  2. We will use length == 0 to determine that log has been entirely
  //     flushed. log_output_msg_process() always calls log_output_flush() but in immediate mode
  //     there is no buffer filled to be flushed so the length will be zero
  if (length > 0 && mflt_ctx->write_idx < sizeof(s_zephyr_render_buf)) {
      s_zephyr_render_buf[mflt_ctx->write_idx] = data[0];
      mflt_ctx->write_idx++;
  }

  const bool flush = (length == 0);
  if (!flush) {
      return (int)length;
  }

  // Strip EOL characters at end of log since we are storing _lines_
  save_length = mflt_ctx->write_idx;
  while ((save_length - 1) > 0) {
    char c = s_zephyr_render_buf[save_length - 1];
    if ((c == '\r') || (c == '\n')) {
      save_length--;
      continue;
    }
    break;
  }
  data = &s_zephyr_render_buf[0];
#endif

  // Note: Context should always be populated via our call to log_output_ctx_set() above.
  // Assert to catch any behavior changes in future versions of Zephyr
  MEMFAULT_SDK_ASSERT(mflt_ctx != NULL);
  memfault_log_save_preformatted_nolock(mflt_ctx->memfault_level, data, save_length);

  return (int) length;
}
