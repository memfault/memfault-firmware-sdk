//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! A simple RAM backed logging storage implementation. When a device crashes and the memory region
//! is collected using the panics component, the logs will be decoded and displayed in the Memfault
//! cloud UI.

#include "memfault/core/log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "memfault/core/compiler.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/math.h"
#include "memfault/core/platform/debug_log.h"
#include "memfault/core/platform/overrides.h"
#include "memfault/util/circular_buffer.h"

#define MEMFAULT_RAM_LOGGER_VERSION 1

#ifndef MEMFAULT_RAM_LOGGER_DEFAULT_MIN_LOG_LEVEL
  #define MEMFAULT_RAM_LOGGER_DEFAULT_MIN_LOG_LEVEL kMemfaultPlatformLogLevel_Info
#endif

typedef struct {
  uint8_t version;
  bool enabled;
  eMemfaultPlatformLogLevel min_log_level;
  sMfltCircularBuffer circ_buffer;
} sMfltRamLogger;

static sMfltRamLogger s_memfault_ram_logger = {
  .enabled = false,
};

typedef enum {
  kMemfaultLogRecordType_Preformatted = 0,

  kMemfaultLogRecordType_NumTypes,
} eMemfaultLogRecordType;

typedef MEMFAULT_PACKED_STRUCT {
  // data about the message stored (details below)
  uint8_t hdr;
  // the length of the msg
  uint8_t len;
  // underlying message
  uint8_t msg[];
} sMfltRamLogEntry;

// Note: We do not use bitfields here to avoid portability complications on the decode side since
// alignment of bitfields as well as the order of bitfields is left up to the compiler per the C
// standard.
//
// Header Layout:
// 0brrrr.tlll
// where
//  r = rsvd
//  t = type (0 = formatted log)
//  l = log level (eMemfaultPlatformLogLevel)

#define MEMFAULT_LOG_HDR_LEVEL_POS  0
#define MEMFAULT_LOG_HDR_LEVEL_MASK 0x07u
#define MEMFAULT_LOG_HDR_TYPE_POS   3u
#define MEMFAULT_LOG_HDR_TYPE_MASK  0x08u

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

  // Expire oldest logs until there is enough room available
  while (tot_read_space != 0) {
    sMfltRamLogEntry curr_entry = { 0 };
    memfault_circular_buffer_read(circ_bufp, 0, &curr_entry, sizeof(curr_entry));
    const size_t space_to_free = curr_entry.len + sizeof(curr_entry);
    memfault_circular_buffer_consume(circ_bufp, space_to_free);
    bytes_needed -= space_to_free;
    if (bytes_needed <= 0) {
      return true;
    }
    tot_read_space = memfault_circular_buffer_get_read_size(circ_bufp);
  }

  return false; // should be unreachable
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

void memfault_log_save(eMemfaultPlatformLogLevel level, const char *fmt, ...) {
  if (!prv_should_log(level)) {
    return;
  }

  char log_buf[MEMFAULT_LOG_MAX_LINE_SAVE_LEN + 1];

  va_list args;
  va_start(args, fmt);
  const size_t available_space = sizeof(log_buf);
  const int rv = vsnprintf(log_buf, available_space, fmt, args);
  va_end(args);

  if (rv <= 0) {
    return;
  }

  size_t bytes_written = (size_t)rv;
  if (bytes_written >= available_space) {
    bytes_written = available_space - 1;
  }
  memfault_log_save_preformatted(level, log_buf, bytes_written);
}

void memfault_log_save_preformatted(eMemfaultPlatformLogLevel level,
                                    const char *log, size_t log_len) {
  if (!prv_should_log(level)) {
    return;
  }

  const size_t bytes_needed = sizeof(sMfltRamLogEntry) + log_len;
  memfault_lock();
  {
    sMfltCircularBuffer *circ_bufp = &s_memfault_ram_logger.circ_buffer;
    const bool space_free = prv_try_free_space(circ_bufp, bytes_needed);
    if (space_free) {
        sMfltRamLogEntry entry = {
          .len = (uint8_t)MEMFAULT_MIN(log_len, MEMFAULT_LOG_MAX_LINE_SAVE_LEN),
          .hdr = prv_build_header(level, kMemfaultLogRecordType_Preformatted),
        };
        memfault_circular_buffer_write(circ_bufp, &entry, sizeof(entry));
        memfault_circular_buffer_write(circ_bufp, log, log_len);
    }
  }
  memfault_unlock();
}

bool memfault_log_boot(void *storage_buffer, size_t buffer_len) {
  if (storage_buffer == NULL || buffer_len == 0 || s_memfault_ram_logger.enabled) {
    return false;
  }
  s_memfault_ram_logger = (sMfltRamLogger) {
    .version = MEMFAULT_RAM_LOGGER_VERSION,
    .min_log_level = MEMFAULT_RAM_LOGGER_DEFAULT_MIN_LOG_LEVEL,
  };

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
