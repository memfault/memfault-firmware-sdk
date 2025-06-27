//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Memfault Zephyr FS Log Data Source Backend

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include "memfault/components.h"
#include "memfault/core/data_packetizer_source.h"

LOG_MODULE_REGISTER(mfltfslog);

MEMFAULT_STATIC_ASSERT(sizeof(CONFIG_MEMFAULT_FS_LOG_EXPORT_TEMPDIR) > 1,
                       "CONFIG_MEMFAULT_FS_LOG_EXPORT_TEMPDIR must be set by the user");

#define MEMFAULT_FS_LOG_EXPORT_STAGED_FILE CONFIG_MEMFAULT_FS_LOG_EXPORT_TEMPDIR "/staged"
//! Limit path sizes for input/output files. This matches the max in the Zephyr
//! fs log backend.
#define MAX_PATH_LEN 256
#define MEMFAULT_FS_LOG_COPY_SIZE 256

//! Build asserts for configured file path sizes
MEMFAULT_STATIC_ASSERT(sizeof(MEMFAULT_FS_LOG_EXPORT_STAGED_FILE) <= MAX_PATH_LEN,
                       "CONFIG_MEMFAULT_FS_LOG_EXPORT_TEMPDIR is too long for staging file path");
MEMFAULT_STATIC_ASSERT(
  sizeof(CONFIG_LOG_BACKEND_FS_DIR "/" CONFIG_LOG_BACKEND_FS_FILE_PREFIX ".1234") <= MAX_PATH_LEN,
  "CONFIG_LOG_BACKEND_FS_DIR and CONFIG_LOG_BACKEND_FS_FILE_PREFIX are too long for file path");

// Scratch buffer to reduce stack/heap pressure
char s_mflt_fs_log_scratch[MAX_PATH_LEN];

static int prv_mkdir_if_needed(const char *path) {
  // Check if the directory already exists
  struct fs_dirent entry;
  int rc = fs_stat(path, &entry);
  if (rc == 0 && entry.type == FS_DIR_ENTRY_DIR) {
    return 0;  // Directory already exists
  }

  // Attempt to create the directory
  rc = fs_mkdir(path);
  if (rc != 0 && rc != -EEXIST) {
    // If it fails and it's not "already exists", report the error
    LOG_ERR("Failed to create directory '%s': %d\n", path, rc);
  }
  return rc;
}

// Create directory recursively if it doesn't exit
static int prv_mkdir_recursive(const char *path) {
  snprintf(s_mflt_fs_log_scratch, sizeof(s_mflt_fs_log_scratch), "%s", path);

  // Skip the leading root '/'
  char *p = s_mflt_fs_log_scratch + 1;
  // Find the first '/' after the mount point
  while (*p && *p != '/') {
    p++;
  }
  if (*p == '\0') {
    // No '/' found, just create the directory
    return prv_mkdir_if_needed(path);
  }
  // Now iterate from the next character after the '/'
  p++;
  for (; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      int rc = prv_mkdir_if_needed(s_mflt_fs_log_scratch);
      if (rc < 0 && errno != EEXIST) {
        return -1;
      }
      *p = '/';
    }
  }
  return prv_mkdir_if_needed(path);
}

//! Zephyr libc include is very stubborn about excluding strdup() definition.
//! It's available at link time for newlib + picolibc, but is difficult to force
//! the header file to include it without impacting other compilation units.
//! Instead, define a simple version here.
static char *prv_strndup(const char *s, size_t max_len) {
  size_t len = strnlen(s, max_len) + 1;
  char *copy = malloc(len);
  if (copy) {
    memcpy(copy, s, len);
  }
  return copy;
}

static const char *mimetypes[] = { MEMFAULT_CDR_TEXT };

static sMemfaultCdrMetadata s_cdr_metadata = {
  .start_time = {
    .type = kMemfaultCurrentTimeType_Unknown,
  },
  .mimetypes = mimetypes,
  .num_mimetypes = MEMFAULT_ARRAY_SIZE(mimetypes),

  .data_size_bytes = 0,
  .duration_ms = 0,

  .collection_reason = "Zephyr FS Log Export",
};

// called to see if there's any data available; uses the *metadata output to set
// the header fields in the chunked message sent to Memfault
static bool prv_has_cdr_cb(sMemfaultCdrMetadata *metadata) {
  *metadata = s_cdr_metadata;
  return s_cdr_metadata.data_size_bytes > 0;
}

// called by the packetizer to read up to .data_size_bytes of CDR data
static bool prv_read_data_cb(uint32_t offset, void *data, size_t data_len) {
  const size_t read_len = MEMFAULT_MIN(data_len, s_cdr_metadata.data_size_bytes - offset);

  // Read bytes out of the staged file
  struct fs_file_t staged_file;
  fs_file_t_init(&staged_file);
  int rc = fs_open(&staged_file, MEMFAULT_FS_LOG_EXPORT_STAGED_FILE, FS_O_READ);
  if (rc != 0) {
    LOG_ERR("Failed to open staged file '%s': %d\n", MEMFAULT_FS_LOG_EXPORT_STAGED_FILE, rc);
    return false;
  }
  rc = fs_seek(&staged_file, offset, FS_SEEK_SET);
  if (rc != 0) {
    LOG_ERR("Failed to seek in staged file '%s': %d\n", MEMFAULT_FS_LOG_EXPORT_STAGED_FILE, rc);
    fs_close(&staged_file);
    return false;
  }
  rc = fs_read(&staged_file, data, read_len);
  if (rc < 0) {
    LOG_ERR("Failed to read from staged file '%s': %d\n", MEMFAULT_FS_LOG_EXPORT_STAGED_FILE, rc);
    fs_close(&staged_file);
    return false;
  } else if ((size_t)rc < read_len) {
    // If we read less than requested, it means we prematurely reached the end
    // of the file. Shouldn't happen.
    LOG_ERR("Read less than requested from staged file '%s': %d < %zu\n",
            MEMFAULT_FS_LOG_EXPORT_STAGED_FILE, rc, read_len);
  }

  fs_close(&staged_file);

  return true;
}

// called when all data has been drained from the read callback
static void prv_mark_cdr_read_cb(void) {
  fs_unlink(MEMFAULT_FS_LOG_EXPORT_STAGED_FILE);
  s_cdr_metadata.data_size_bytes = 0;
}

// Set up the CDR callback functions
static const sMemfaultCdrSourceImpl s_mflt_fs_log_cdr_source = {
  .has_cdr_cb = prv_has_cdr_cb,
  .read_data_cb = prv_read_data_cb,
  .mark_cdr_read_cb = prv_mark_cdr_read_cb,
};

static void prv_enable_cdr(size_t data_size_bytes) {
  sMemfaultCurrentTime timestamp;
  if (memfault_platform_time_get_current(&timestamp)) {
    s_cdr_metadata.start_time = timestamp;
  } else {
    s_cdr_metadata.start_time.type = kMemfaultCurrentTimeType_Unknown;
  }

  s_cdr_metadata.data_size_bytes = data_size_bytes;

  memfault_cdr_register_source(&s_mflt_fs_log_cdr_source);
}

size_t memfault_fs_log_trigger_export(void) {
  // Get a list of all files in the fs log storage directory
  char *file_list[CONFIG_LOG_BACKEND_FS_FILES_LIMIT];
  size_t file_count = 0;
  struct fs_dir_t dir;
  struct fs_dirent entry;
  fs_dir_t_init(&dir);
  int rc = fs_opendir(&dir, CONFIG_LOG_BACKEND_FS_DIR);
  if (rc != 0) {
    return 0;
  }
  while (fs_readdir(&dir, &entry) == 0 && entry.name[0] != 0) {
    if (strncmp(entry.name, CONFIG_LOG_BACKEND_FS_FILE_PREFIX,
                strlen(CONFIG_LOG_BACKEND_FS_FILE_PREFIX)) == 0) {
      if (file_count < CONFIG_LOG_BACKEND_FS_FILES_LIMIT) {
        file_list[file_count] = prv_strndup(entry.name, MAX_PATH_LEN);
        file_count++;
      }
    }
  }
  fs_closedir(&dir);

  // Sort the file list, accounting for wrap-around and rollover.
  //
  // For example, the set of files can be a simple sequence:
  // log.0123, log.0124, log.0125, ..., log.0132
  //
  // Or it can wrap around:
  // log.9998, log.9999, log.0000, log.0002, ..., log.0007
  //
  // In both cases, we want to copy file content starting from the oldest file
  // to the newest file. All the file data will be appended to a new file,
  // "CONFIG_MEMFAULT_FS_LOG_EXPORT_TEMPDIR/staged"

  // Extract numeric indices from filenames for proper sorting
  struct sMfltFsLogIndexedFile {
    char *name;      // File name
    uint32_t index;  // Numeric index extracted from the file name
  };
  struct sMfltFsLogIndexedFile indexed_files[CONFIG_LOG_BACKEND_FS_FILES_LIMIT];

  for (size_t i = 0; i < file_count; i++) {
    indexed_files[i].name = file_list[i];
    // Extract index from filename (e.g., "log.0123" -> 123)
    const char *index_str = file_list[i] + strlen(CONFIG_LOG_BACKEND_FS_FILE_PREFIX);
    indexed_files[i].index = (uint32_t)atoi(index_str);
  }

  // Sort the files by index
  for (size_t i = 0; i < file_count - 1; i++) {
    for (size_t j = i + 1; j < file_count; j++) {
      if (indexed_files[i].index > indexed_files[j].index) {
        // Swap if out of order
        struct sMfltFsLogIndexedFile temp = indexed_files[i];  // Temporary struct to hold the swap
        indexed_files[i] = indexed_files[j];
        indexed_files[j] = temp;
      }
    }
  }

  // Find the oldest file (handle wrap-around)
  size_t oldest_idx = 0;
  if (file_count > 1) {
    // Check for wrap-around by finding large gaps in sequence
    for (size_t i = 1; i < file_count; i++) {
      uint32_t prev_index = indexed_files[(i - 1) % file_count].index;
      uint32_t curr_index = indexed_files[i].index;

      // If there's a step of more than 1, the file after the gap is oldest.
      // For example, if we have:
      // log.9998, log.9999, log.0000, log.0002, ..., log.0007
      // then the positive gap >1 is between log.0007 and log.9998,
      // and the oldest file is log.9998.
      if (prev_index < curr_index && (curr_index - prev_index) > 1) {
        oldest_idx = i;
        break;
      }
    }
  }

  // Create the staged file by concatenating all log files in chronological order
  prv_mkdir_recursive(CONFIG_MEMFAULT_FS_LOG_EXPORT_TEMPDIR);
  struct fs_file_t staged_file;
  fs_file_t_init(&staged_file);
  rc = fs_open(&staged_file, MEMFAULT_FS_LOG_EXPORT_STAGED_FILE,
               FS_O_CREATE | FS_O_WRITE | FS_O_TRUNC);
  size_t staged_file_size = 0;
  if (rc == 0) {
    // Copy files starting from oldest
    for (size_t i = 0; i < file_count; i++) {
      size_t file_idx = (oldest_idx + i) % file_count;

      snprintf(s_mflt_fs_log_scratch, sizeof(s_mflt_fs_log_scratch), "%s/%s",
               CONFIG_LOG_BACKEND_FS_DIR, indexed_files[file_idx].name);

      struct fs_file_t src_file;
      fs_file_t_init(&src_file);
      if (fs_open(&src_file, s_mflt_fs_log_scratch, FS_O_READ) == 0) {
        ssize_t nread;
        while ((nread = fs_read(&src_file, s_mflt_fs_log_scratch, sizeof(s_mflt_fs_log_scratch))) >
               0) {
          fs_write(&staged_file, s_mflt_fs_log_scratch, nread);
        }
        fs_close(&src_file);
      }
    }
    // Get the size of the staged file
    staged_file_size = fs_tell(&staged_file);

    fs_close(&staged_file);

    if (staged_file_size > 0) {
      prv_enable_cdr(staged_file_size);
    } else {
      LOG_ERR("Staged file is empty or could not be created");
    }
  }

  // Clean up allocated memory
  for (size_t i = 0; i < file_count; ++i) {
    free(file_list[i]);
  }

  return staged_file_size;
}

//! In the future, the log data will be exported as a log message
// static bool prv_export_fs_log_data(uint32_t offset, void *buf, size_t buf_len) {
//   return true;
// }

// // Data source implementation for Memfault packetizer
// static bool prv_fs_log_has_data(size_t *total_size) {
//   // Return the size of the staged file
//   struct fs_dirent entry;
//   int rc = fs_stat(MEMFAULT_FS_LOG_EXPORT_STAGED_FILE, &entry);
//   if (rc != 0) {
//     *total_size = 0;
//     return false;
//   }

//   *total_size = entry.size;
//   return entry.size > 0;
// }

// static void prv_fs_log_mark_sent(void) {
//   // Clean up the staged file
//   fs_unlink(MEMFAULT_FS_LOG_EXPORT_STAGED_FILE);
// }

// const sMemfaultDataSourceImpl g_memfault_fs_log_data_source = {
//   .has_more_msgs_cb = prv_fs_log_has_data,
//   .read_msg_cb = prv_export_fs_log_data,
//   .mark_msg_read_cb = prv_fs_log_mark_sent,
// };
