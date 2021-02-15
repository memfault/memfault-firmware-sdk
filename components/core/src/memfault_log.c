//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! A simple RAM backed logging storage implementation. When a device crashes and the memory region
//! is collected using the panics component, the logs will be decoded and displayed in the Memfault
//! cloud UI.

#include "memfault/core/log.h"
#include "memfault/core/log_impl.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "memfault/config.h"
#include "memfault/core/compiler.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/math.h"
#include "memfault/core/platform/debug_log.h"
#include "memfault/core/platform/overrides.h"
#include "memfault/util/circular_buffer.h"
#include "memfault/util/crc16_ccitt.h"

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
// 0brxxx.tlll
// where
//  r = read (1 if the message has been read, 0 otherwise)
//  x = rsvd
//  t = type (0 = formatted log)
//  l = log level (eMemfaultPlatformLogLevel)

#define MEMFAULT_LOG_HDR_LEVEL_POS  0
#define MEMFAULT_LOG_HDR_LEVEL_MASK 0x07u
#define MEMFAULT_LOG_HDR_TYPE_POS   3u
#define MEMFAULT_LOG_HDR_TYPE_MASK  0x08u
#define MEMFAULT_LOG_HDR_READ_MASK  0x80u

static eMemfaultPlatformLogLevel prv_get_level_from_hdr(uint8_t hdr) {
  return (eMemfaultPlatformLogLevel)((hdr & MEMFAULT_LOG_HDR_LEVEL_MASK) >> MEMFAULT_LOG_HDR_LEVEL_POS);
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

static bool prv_read_log(sMemfaultLog *log) {
  if (s_memfault_ram_logger.dropped_msg_count) {
    log->level = kMemfaultPlatformLogLevel_Warning;
    const int rv = snprintf(log->msg, sizeof(log->msg), "... %d messages dropped ...",
                                 (int)s_memfault_ram_logger.dropped_msg_count);
    log->msg_len = (rv <= 0)  ? 0 : MEMFAULT_MIN((uint32_t)rv, sizeof(log->msg) - 1);
    s_memfault_ram_logger.dropped_msg_count = 0;
    return true;
  }

  sMfltCircularBuffer *circ_bufp = &s_memfault_ram_logger.circ_buffer;
  sMfltRamLogEntry curr_entry = { 0 };
  uint32_t read_offset = s_memfault_ram_logger.log_read_offset;
  if (!memfault_circular_buffer_read(circ_bufp, read_offset, &curr_entry, sizeof(curr_entry))) {
    return false;
  }

  // Note: At this point, the memfault_circular_buffer_* return calls below should never fail. A
  // failure is indicative of memory corruption (e.g calls taking place from multiple tasks without
  // having implemented memfault_lock() / memfault_unlock())

  // mark the message as read
  curr_entry.hdr |= MEMFAULT_LOG_HDR_READ_MASK;

  const size_t offset_from_end = memfault_circular_buffer_get_read_size(circ_bufp) - read_offset;
  if (!memfault_circular_buffer_write_at_offset(
          circ_bufp, offset_from_end, &curr_entry, sizeof(curr_entry))) {
    return false;
  }


  read_offset += sizeof(curr_entry);
  if (!memfault_circular_buffer_read(circ_bufp, read_offset, log->msg, curr_entry.len)) {
    return false;
  }

  log->msg[curr_entry.len] = '\0';
  log->level = prv_get_level_from_hdr(curr_entry.hdr);
  log->msg_len = curr_entry.len;
  s_memfault_ram_logger.log_read_offset = read_offset + curr_entry.len;
  return true;
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

  bool log_written = false;
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
        log_written = true;
    }
  }
  memfault_unlock();

  if (log_written) {
    memfault_log_handle_saved_callback();
  }
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
