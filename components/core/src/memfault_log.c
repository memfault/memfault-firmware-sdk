//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! A simple RAM backed logging storage implementation. When a device crashes and the memory region
//! is collected using the panics component, the logs will be decoded and displayed in the Memfault
//! cloud UI.

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "memfault/config.h"
#include "memfault/core/compact_log_serializer.h"
#include "memfault/core/compiler.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/log.h"
#include "memfault/core/log_impl.h"
#include "memfault/core/math.h"
#include "memfault/core/platform/debug_log.h"
#include "memfault/core/platform/overrides.h"
#include "memfault/core/sdk_assert.h"
#include "memfault/util/base64.h"
#include "memfault/util/circular_buffer.h"
#include "memfault/util/crc16_ccitt.h"
#include "memfault_log_private.h"

#if MEMFAULT_LOG_DATA_SOURCE_ENABLED
#include "memfault_log_data_source_private.h"
#endif

#define MEMFAULT_RAM_LOGGER_VERSION 1

typedef struct MfltLogStorageInfo {
  void *storage;
  size_t len;
  uint16_t crc16;
} sMfltLogStorageRegionInfo;

typedef struct {
  uint8_t version;
  bool enabled;
  // the minimum log level level that will be saved
  // Can be changed with memfault_log_set_min_save_level()
  eMemfaultPlatformLogLevel min_log_level;
  uint32_t log_read_offset;
  // The number of messages that were flushed without ever being read. If memfault_log_read() is
  // not used by a platform, this will be equivalent to the number of messages logged since boot
  // that are no longer in the log buffer.
  uint32_t dropped_msg_count;
  sMfltCircularBuffer circ_buffer;
  // When initialized we keep track of the user provided storage buffer and crc the location +
  // size. When the system crashes we can check to see if this info has been corrupted in any way
  // before trying to collect the region.
  sMfltLogStorageRegionInfo region_info;
} sMfltRamLogger;

static sMfltRamLogger s_memfault_ram_logger = {
  .enabled = false,
};

static uint16_t prv_compute_log_region_crc16(void) {
  return memfault_crc16_ccitt_compute(
      MEMFAULT_CRC16_CCITT_INITIAL_VALUE, &s_memfault_ram_logger.region_info,
      offsetof(sMfltLogStorageRegionInfo, crc16));
}

bool memfault_log_get_regions(sMemfaultLogRegions *regions) {
  if (!s_memfault_ram_logger.enabled) {
    return false;
  }

  const sMfltLogStorageRegionInfo *region_info = &s_memfault_ram_logger.region_info;
  const uint16_t current_crc16 = prv_compute_log_region_crc16();
  if (current_crc16 != region_info->crc16) {
    return false;
  }

  *regions = (sMemfaultLogRegions) {
    .region = {
      {
        .region_start = &s_memfault_ram_logger,
        .region_size = sizeof(s_memfault_ram_logger),
      },
      {
        .region_start = region_info->storage,
        .region_size = region_info->len,
      }
    }
  };
  return true;
}

static uint8_t prv_build_header(eMemfaultPlatformLogLevel level, eMemfaultLogRecordType type) {
  MEMFAULT_STATIC_ASSERT(kMemfaultPlatformLogLevel_NumLevels <= 8,
                         "Number of log levels exceed max number that log module can track");
  MEMFAULT_STATIC_ASSERT(kMemfaultLogRecordType_NumTypes <= 2,
                         "Number of log types exceed max number that log module can track");

  const uint8_t level_field = (level << MEMFAULT_LOG_HDR_LEVEL_POS) & MEMFAULT_LOG_HDR_LEVEL_MASK;
  const uint8_t type_field = (type << MEMFAULT_LOG_HDR_TYPE_POS) & MEMFAULT_LOG_HDR_TYPE_MASK;
  return level_field | type_field;
}

void memfault_log_set_min_save_level(eMemfaultPlatformLogLevel min_log_level) {
  s_memfault_ram_logger.min_log_level = min_log_level;
}

static bool prv_try_free_space(sMfltCircularBuffer *circ_bufp, int bytes_needed) {
  const size_t bytes_free = memfault_circular_buffer_get_write_size(circ_bufp);
  bytes_needed -= bytes_free;
  if (bytes_needed <= 0) {
    // no work to do, there is already enough space available
    return true;
  }

  size_t tot_read_space = memfault_circular_buffer_get_read_size(circ_bufp);
  if (bytes_needed > (int)tot_read_space) {
    // Even if we dropped all the data in the buffer there would not be enough space
    // This means the message is larger than the storage area we have allocated for the buffer
    return false;
  }

#if MEMFAULT_LOG_DATA_SOURCE_ENABLED
  if (memfault_log_data_source_has_been_triggered()) {
    // Don't allow expiring old logs. memfault_log_trigger_collection() has been
    // called, so we're in the process of uploading the logs in the buffer.
    return false;
  }
#endif

  // Expire oldest logs until there is enough room available
  while (tot_read_space != 0) {
    sMfltRamLogEntry curr_entry = { 0 };
    memfault_circular_buffer_read(circ_bufp, 0, &curr_entry, sizeof(curr_entry));
    const size_t space_to_free = curr_entry.len + sizeof(curr_entry);

    if ((curr_entry.hdr & MEMFAULT_LOG_HDR_READ_MASK) != 0) {
      s_memfault_ram_logger.log_read_offset -= space_to_free;
    } else {
      // We are removing a message that was not read via memfault_log_read().
      s_memfault_ram_logger.dropped_msg_count++;
    }

    memfault_circular_buffer_consume(circ_bufp, space_to_free);
    bytes_needed -= space_to_free;
    if (bytes_needed <= 0) {
      return true;
    }
    tot_read_space = memfault_circular_buffer_get_read_size(circ_bufp);
  }

  return false; // should be unreachable
}

static void prv_iterate(MemfaultLogIteratorCallback callback, sMfltLogIterator *iter) {
  sMfltCircularBuffer *const circ_bufp = &s_memfault_ram_logger.circ_buffer;
  bool should_continue = true;
  while (should_continue) {
    if (!memfault_circular_buffer_read(
        circ_bufp, iter->read_offset, &iter->entry, sizeof(iter->entry))) {
      return;
    }

    // Note: At this point, the memfault_log_iter_update_entry(),
    // memfault_log_entry_get_msg_pointer() calls made from the callback should never fail.
    // A failure is indicative of memory corruption (e.g calls taking place from multiple tasks
    // without having implemented memfault_lock() / memfault_unlock())

    should_continue = callback(iter);
    iter->read_offset += sizeof(iter->entry) + iter->entry.len;
  }
}

void memfault_log_iterate(MemfaultLogIteratorCallback callback, sMfltLogIterator *iter) {
  memfault_lock();
  prv_iterate(callback, iter);
  memfault_unlock();
}

bool memfault_log_iter_update_entry(sMfltLogIterator *iter) {
  sMfltCircularBuffer *const circ_bufp = &s_memfault_ram_logger.circ_buffer;
  const size_t offset_from_end =
      memfault_circular_buffer_get_read_size(circ_bufp) - iter->read_offset;
  return memfault_circular_buffer_write_at_offset(
      circ_bufp, offset_from_end, &iter->entry, sizeof(iter->entry));
}

bool memfault_log_iter_copy_msg(sMfltLogIterator *iter, MemfaultLogMsgCopyCallback callback) {
  sMfltCircularBuffer *const circ_bufp = &s_memfault_ram_logger.circ_buffer;
  return memfault_circular_buffer_read_with_callback(
    circ_bufp, iter->read_offset + sizeof(iter->entry), iter->entry.len, iter,
    (MemfaultCircularBufferReadCallback)callback);
}

typedef struct {
  sMemfaultLog *log;
  bool has_log;
} sMfltReadLogCtx;

static bool prv_read_log_iter_callback(sMfltLogIterator *iter) {
  sMfltReadLogCtx *const ctx = (sMfltReadLogCtx *)iter->user_ctx;
  sMfltCircularBuffer *const circ_bufp = &s_memfault_ram_logger.circ_buffer;

  // mark the message as read
  iter->entry.hdr |= MEMFAULT_LOG_HDR_READ_MASK;
  if (!memfault_log_iter_update_entry(iter)) {
    return false;
  }

  if (!memfault_circular_buffer_read(
      circ_bufp, iter->read_offset + sizeof(iter->entry), ctx->log->msg, iter->entry.len)) {
    return false;
  }

  ctx->log->msg[iter->entry.len] = '\0';
  ctx->log->level = memfault_log_get_level_from_hdr(iter->entry.hdr);
  ctx->log->type = memfault_log_get_type_from_hdr(iter->entry.hdr);
  ctx->log->msg_len = iter->entry.len;
  ctx->has_log = true;
  return false;
}

static bool prv_read_log(sMemfaultLog *log) {
  if (s_memfault_ram_logger.dropped_msg_count) {
    log->level = kMemfaultPlatformLogLevel_Warning;
    const int rv = snprintf(log->msg, sizeof(log->msg), "... %d messages dropped ...",
                                 (int)s_memfault_ram_logger.dropped_msg_count);
    log->msg_len = (rv <= 0)  ? 0 : MEMFAULT_MIN((uint32_t)rv, sizeof(log->msg) - 1);
    log->type = kMemfaultLogRecordType_Preformatted;
    s_memfault_ram_logger.dropped_msg_count = 0;
    return true;
  }

  sMfltReadLogCtx user_ctx = {
    .log = log
  };

  sMfltLogIterator iter = {
    .read_offset = s_memfault_ram_logger.log_read_offset,

    .user_ctx = &user_ctx
  };

  prv_iterate(prv_read_log_iter_callback, &iter);
  s_memfault_ram_logger.log_read_offset = iter.read_offset;
  return user_ctx.has_log;
}

bool memfault_log_read(sMemfaultLog *log) {
  if (!s_memfault_ram_logger.enabled || (log == NULL)) {
    return false;
  }

  memfault_lock();
  const bool found_unread_log = prv_read_log(log);
  memfault_unlock();

  return found_unread_log;
}

MEMFAULT_WEAK void memfault_log_export_msg(MEMFAULT_UNUSED eMemfaultPlatformLogLevel level,
                                           const char *msg, MEMFAULT_UNUSED size_t msg_len) {
  memfault_platform_log_raw("%s", msg);
}

void memfault_log_export_log(sMemfaultLog *log) {
  MEMFAULT_SDK_ASSERT(log != NULL);

  char base64[MEMFAULT_LOG_EXPORT_BASE64_CHUNK_MAX_LEN];
  size_t log_read_offset = 0;

  switch (log->type) {
    case kMemfaultLogRecordType_Compact:
      while (log_read_offset < log->msg_len) {
        memcpy(base64, MEMFAULT_LOG_EXPORT_BASE64_CHUNK_PREFIX,
               MEMFAULT_LOG_EXPORT_BASE64_CHUNK_PREFIX_LEN);

        size_t log_read_len =
          MEMFAULT_MIN(log->msg_len - log_read_offset, MEMFAULT_LOG_EXPORT_CHUNK_MAX_LEN);
        size_t write_offset = MEMFAULT_LOG_EXPORT_BASE64_CHUNK_PREFIX_LEN;

        memfault_base64_encode(&log->msg[log_read_offset], log_read_len, &base64[write_offset]);
        log_read_offset += log_read_len;
        write_offset += MEMFAULT_BASE64_ENCODE_LEN(log_read_len);

        memcpy(&base64[write_offset], MEMFAULT_LOG_EXPORT_BASE64_CHUNK_SUFFIX,
               MEMFAULT_LOG_EXPORT_BASE64_CHUNK_SUFFIX_LEN);
        write_offset += MEMFAULT_LOG_EXPORT_BASE64_CHUNK_SUFFIX_LEN;

        base64[write_offset] = '\0';

        memfault_log_export_msg(log->level, base64, write_offset);
      }
      break;
    case kMemfaultLogRecordType_Preformatted:
      memfault_log_export_msg(log->level, log->msg, log->msg_len);
      break;
    case kMemfaultLogRecordType_NumTypes:  // silences -Wswitch-enum
    default:
      break;
  }
}

void memfault_log_export_logs(void) {
  while (1) {
    // the TI ARM compiler warns about enumerated type mismatch in this
    // zero-initializer, but we depend on the C99 spec (vs. the gcc extension,
    // empty brace {}), so suppress the diagnostic here. Clang throws an invalid
    // -Wmissing-field-initializers warning if we just cast, unfortunately.
    #if defined(__TI_ARM__)
      #pragma diag_push
      #pragma diag_remark 190
    #endif
    #if defined(__CC_ARM)
      #pragma push
      // This armcc diagnostic is technically violating the C standard, which
      // _explicitly_ requires enums to be type-equivalent to ints. See ISO/IEC
      // 9899:TC3 6.2.5.16, and specifically 6.4.4.3, which states: "An
      // identifier declared as an enumeration constant has type int."
      #pragma diag_suppress 188  // enumerated type mixed with another type
    #endif
    sMemfaultLog log = {0};
    #if defined(__TI_ARM__)
      #pragma diag_pop
    #endif
    #if defined(__CC_ARM)
      #pragma pop
    #endif

    const bool log_found = memfault_log_read(&log);
    if (!log_found) {
      break;
    }

    memfault_log_export_log(&log);
  }
}

static bool prv_should_log(eMemfaultPlatformLogLevel level) {
  if (!s_memfault_ram_logger.enabled) {
    return false;
  }

  if (level < s_memfault_ram_logger.min_log_level) {
    return false;
  }

  return true;
}

//! Stub implementation that a user of the SDK can override. See header for more details.
MEMFAULT_WEAK void memfault_log_handle_saved_callback(void) {
  return;
}

void memfault_vlog_save(eMemfaultPlatformLogLevel level, const char *fmt, va_list args) {
  if (!prv_should_log(level)) {
    return;
  }

  char log_buf[MEMFAULT_LOG_MAX_LINE_SAVE_LEN + 1];

  const size_t available_space = sizeof(log_buf);
  const int rv = vsnprintf(log_buf, available_space, fmt, args);

  if (rv <= 0) {
    return;
  }

  size_t bytes_written = (size_t)rv;
  if (bytes_written >= available_space) {
    bytes_written = available_space - 1;
  }

  memfault_log_save_preformatted(level, log_buf, bytes_written);
}

void memfault_log_save(eMemfaultPlatformLogLevel level, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  memfault_vlog_save(level, fmt, args);
  va_end(args);
}

static void prv_log_save(eMemfaultPlatformLogLevel level, const void *log, size_t log_len,
                         eMemfaultLogRecordType log_type, bool should_lock) {
  if (!prv_should_log(level)) {
    return;
  }

  bool log_written = false;
  const size_t truncated_log_len = MEMFAULT_MIN(log_len, MEMFAULT_LOG_MAX_LINE_SAVE_LEN);
  const size_t bytes_needed = sizeof(sMfltRamLogEntry) + truncated_log_len;
  if (should_lock) {
    memfault_lock();
  }
  {
    sMfltCircularBuffer *circ_bufp = &s_memfault_ram_logger.circ_buffer;
    const bool space_free = prv_try_free_space(circ_bufp, (int)bytes_needed);
    if (space_free) {
        sMfltRamLogEntry entry = {
          .len = (uint8_t)truncated_log_len,
          .hdr = prv_build_header(level, log_type),
        };
        memfault_circular_buffer_write(circ_bufp, &entry, sizeof(entry));
        memfault_circular_buffer_write(circ_bufp, log, truncated_log_len);
        log_written = true;
    }
  }
  if (should_lock) {
    memfault_unlock();
  }

  if (log_written) {
    memfault_log_handle_saved_callback();
  }
}

#if MEMFAULT_COMPACT_LOG_ENABLE

void memfault_compact_log_save(eMemfaultPlatformLogLevel level, uint32_t log_id,
                               uint32_t compressed_fmt, ...) {
  char log_buf[MEMFAULT_LOG_MAX_LINE_SAVE_LEN + 1];

  sMemfaultCborEncoder encoder;
  memfault_cbor_encoder_init(&encoder, memfault_cbor_encoder_memcpy_write, log_buf,
                             sizeof(log_buf));

  va_list args;
  va_start(args, compressed_fmt);
  bool success = memfault_vlog_compact_serialize(&encoder, log_id, compressed_fmt, args);
  va_end(args);

  if (!success) {
    return;
  }

  const size_t bytes_written = memfault_cbor_encoder_deinit(&encoder);
  prv_log_save(level, log_buf, bytes_written, kMemfaultLogRecordType_Compact, true);
}

#endif /* MEMFAULT_COMPACT_LOG_ENABLE */


void memfault_log_save_preformatted(eMemfaultPlatformLogLevel level,
                                    const char *log, size_t log_len) {
  prv_log_save(level, log, log_len, kMemfaultLogRecordType_Preformatted, true);
}

void memfault_log_save_preformatted_nolock(eMemfaultPlatformLogLevel level, const char *log,
                                           size_t log_len) {
  prv_log_save(level, log, log_len, kMemfaultLogRecordType_Preformatted, false);
}

bool memfault_log_boot(void *storage_buffer, size_t buffer_len) {
  if (storage_buffer == NULL || buffer_len == 0 || s_memfault_ram_logger.enabled) {
    return false;
  }

  s_memfault_ram_logger = (sMfltRamLogger) {
    .version = MEMFAULT_RAM_LOGGER_VERSION,
    .min_log_level = MEMFAULT_RAM_LOGGER_DEFAULT_MIN_LOG_LEVEL,
    .region_info = {
      .storage = storage_buffer,
      .len = buffer_len,
    },
  };

  s_memfault_ram_logger.region_info.crc16 = prv_compute_log_region_crc16();

  memfault_circular_buffer_init(&s_memfault_ram_logger.circ_buffer, storage_buffer, buffer_len);

  // finally, enable logging
  s_memfault_ram_logger.enabled = true;
  return true;
}

void memfault_log_reset(void) {
  s_memfault_ram_logger = (sMfltRamLogger) {
    .enabled = false,
  };
}

bool memfault_log_booted(void) {
  return s_memfault_ram_logger.enabled;
}
