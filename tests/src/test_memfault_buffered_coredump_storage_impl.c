//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! A fake implementation of coredump storage which makes use of the
//! implementation in memfault/ports/buffered_coredump_storage.h
//! We use this in testing to verify that the buffered writer is working
//! as expected

#include "memfault/ports/buffered_coredump_storage.h"

#include <assert.h>
#include <string.h>

#include "fakes/fake_memfault_buffered_coredump_storage.h"

uint8_t g_ram_backed_storage[MEMFAULT_STORAGE_SIZE];
static size_t s_active_storage_size = MEMFAULT_STORAGE_SIZE;
static bool s_injected_write_failure = false;

void fake_buffered_coredump_inject_write_failure(void) {
  s_injected_write_failure = true;
}

void memfault_platform_coredump_storage_get_info(sMfltCoredumpStorageInfo *info) {
  *info = (sMfltCoredumpStorageInfo) {
    .size = s_active_storage_size
  };
}

bool memfault_platform_coredump_storage_buffered_write(sCoredumpWorkingBuffer *block) {
  if (s_injected_write_failure) {
    s_injected_write_failure = false;
    return false;
  }

  assert(block != NULL);
  // writes should always be at an aligned offset
  assert((block->write_offset % MEMFAULT_COREDUMP_STORAGE_WRITE_SIZE) == 0);
  assert(block->write_offset < s_active_storage_size);
  assert((block->write_offset + sizeof(block->data)) <= s_active_storage_size);

  memcpy(&g_ram_backed_storage[block->write_offset], block->data, sizeof(block->data));
  return true;
}

void fake_buffered_coredump_storage_reset(void) {
  memset(g_ram_backed_storage, 0xff, MEMFAULT_STORAGE_SIZE);
  s_active_storage_size = MEMFAULT_STORAGE_SIZE;
  s_injected_write_failure = false;
  memset(&s_working_buffer_header, 0x0, sizeof(s_working_buffer_header));
  memset(&s_working_buffer, 0x0, sizeof(s_working_buffer));
}

void fake_buffered_coredump_storage_set_size(size_t size) {
  s_active_storage_size = size;
}
