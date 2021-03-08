//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Hooks the Memfault logging API to zephyr's logging system.
//! This is different from the debug log.

#include <memfault/core/log.h>
#include "memfault/core/sdk_assert.h"

#include <logging/log.h>
#include <logging/log_backend.h>
#include <logging/log_output.h>

#include <stddef.h>
#include <stdint.h>

// Can't be zero but should be reasonably sized. See ports/zephyr/Kconfig to change this size.
BUILD_ASSERT(CONFIG_MEMFAULT_LOGGING_RAM_SIZE);

// Define a log_output_mflt_control_block and log_output_mflt that references
// log_output_mflt_control_block via pointer. First arg to LOG_OUTPUT_DEFINE()
// is a token that the C preprocessor uses to create symbol names,
// e.g. struct log_output n and struct log_output_control_block n_control_block.
// Within the log_output_control_block there is a void *ctx we can use to store
// info we need when called by Zephyr by calling log_output_ctx_set().
static uint8_t s_zephyr_render_buf[128];
static int prv_log_out(uint8_t *data, size_t length, void *ctx);
LOG_OUTPUT_DEFINE(log_output_mflt, prv_log_out, s_zephyr_render_buf, sizeof(s_zephyr_render_buf));

// Construct our backend API object. Might need to check how/if we want to support
// put_sync_string(), put_sync_hexdump() and dropped().
static void put(const struct log_backend *const backend, struct log_msg *msg);
static void put_sync_string(const struct log_backend *const backend,
                            struct log_msg_ids src_level, uint32_t timestamp,
                            const char *fmt, va_list ap);
static void put_sync_hexdump(const struct log_backend *const backend,
                             struct log_msg_ids src_level,
                             uint32_t timestamp,
                             const char *metadata,
                             const uint8_t *data, uint32_t length);
static void panic(struct log_backend const *const backend);
static void init(void);
static void dropped(const struct log_backend *const backend, uint32_t cnt);
const struct log_backend_api log_backend_mflt_api = {
  .put              = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : put,
  .put_sync_string  = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? put_sync_string : NULL,
  .put_sync_hexdump = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? put_sync_hexdump : NULL,
  .panic            = panic,
  .init             = init,
  .dropped          = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : dropped,
};

// Define a couple of structs needed by the logging backend infrastructure.
// Binds our log_backend_mflt_api into the logger.
LOG_BACKEND_DEFINE(log_backend_mflt, log_backend_mflt_api, true);

// Tie Memfault's log function to the Zephyr buffer sender. This is *the* connection to Memfault.
static int prv_log_out(uint8_t *data, size_t length, void *ctx) {
  // Note: Context should always be populated. If it is not, flag the log as an _Error
  const eMemfaultPlatformLogLevel log_level = ctx != NULL ? *(eMemfaultPlatformLogLevel*)ctx :
                                                            kMemfaultPlatformLogLevel_Error;
  memfault_log_save_preformatted(log_level, data, length);
  return (int) length;
}


static eMemfaultPlatformLogLevel prv_map_zephyr_level_to_memfault(uint32_t zephyr_level) {
  //     Map             From            To
  return zephyr_level == LOG_LEVEL_ERR ? kMemfaultPlatformLogLevel_Error
       : zephyr_level == LOG_LEVEL_WRN ? kMemfaultPlatformLogLevel_Warning
       : zephyr_level == LOG_LEVEL_INF ? kMemfaultPlatformLogLevel_Info
       :              /* LOG_LEVEL_DBG */kMemfaultPlatformLogLevel_Debug;
}

// *** Below are the implementations for the Zephyr backend API ***

// Zephyr API function. I'm assuming <msg> has been validated by the time put() is called.
static void put(const struct log_backend *const backend, struct log_msg *msg) {
  // Copied flagging from Zephry's ring buffer (rb) implementation.
  const uint32_t flags = IS_ENABLED(CONFIG_LOG_BACKEND_FORMAT_TIMESTAMP)
                       ? LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP | LOG_OUTPUT_FLAG_LEVEL
                       : LOG_OUTPUT_FLAG_LEVEL;

  // Acquire, process (eventually calls prv_data_out()) and release the message.
  log_msg_get(msg);
  const uint32_t zephyr_level = log_msg_level_get(msg);
  if (zephyr_level != LOG_LEVEL_NONE) {
    // We might log so figure out if Memfault logging level for when Zephyr calls us back.
    eMemfaultPlatformLogLevel memfault_level = prv_map_zephyr_level_to_memfault(zephyr_level);
    log_output_ctx_set(&log_output_mflt, &memfault_level);
    log_output_msg_process(&log_output_mflt, msg, flags);
  }
  log_msg_put(msg);
}


static void put_sync_string(const struct log_backend *const backend,
                        struct log_msg_ids src_level, uint32_t timestamp,
                        const char *fmt, va_list ap) {
  const uint32_t flags = IS_ENABLED(CONFIG_LOG_BACKEND_MFLT_SYSLOG_ENABLE)
                       ? LOG_OUTPUT_FLAG_FORMAT_SYSLOG
                       : 0;

  log_output_string(&log_output_mflt, src_level, timestamp, fmt, ap, flags);
}


static void put_sync_hexdump(const struct log_backend *const backend,
                         struct log_msg_ids src_level,
                         uint32_t timestamp,
                         const char *metadata,
                         const uint8_t *data, uint32_t length) {
  const uint32_t flags = IS_ENABLED(CONFIG_LOG_BACKEND_MFLT_SYSLOG_ENABLE)
                       ? LOG_OUTPUT_FLAG_FORMAT_SYSLOG
                       : 0;

  log_output_hexdump(&log_output_mflt, src_level, timestamp, metadata, data, length, flags);
}


static void panic(struct log_backend const *const backend) {
  log_output_flush(&log_output_mflt);
}


// Zephyr will call our init function so we can establish some storage.
static void init(void) {
  // static RAM storage where logs will be stored. Storage can be any size
  // you want but you will want it to be able to hold at least a couple logs.
  static uint8_t mflt_log_buf_storage[CONFIG_MEMFAULT_LOGGING_RAM_SIZE];
  memfault_log_boot(mflt_log_buf_storage, sizeof(mflt_log_buf_storage));
}


static void dropped(const struct log_backend *const backend, uint32_t cnt) {
  ARG_UNUSED(backend);
  log_output_dropped_process(&log_output_mflt, cnt);
}
