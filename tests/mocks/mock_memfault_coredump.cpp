#include "mock_memfault_coredump.h"

#include "CppUTestExt/MockSupport.h"

extern "C" {
  #include <stdbool.h>
  #include <string.h>

  #include "memfault/core/data_packetizer_source.h"
  #include "memfault/panics/coredump.h"
  #include "memfault/panics/platform/coredump.h"
}

uint32_t g_mock_memfault_coredump_total_size = COREDUMP_TOTAL_SIZE;

bool memfault_platform_coredump_storage_read(uint32_t offset, void *data,
                                             size_t read_len) {
  const bool rv = mock().actualCall(__func__)
                        .withUnsignedIntParameter("offset", offset)
                        .withUnsignedIntParameter("read_len", (unsigned int)read_len)
                        .returnBoolValueOrDefault(true);
  if (rv) {
    memset(data, 0xff, read_len);
  }
  return rv;
}

bool memfault_coredump_has_valid_coredump(size_t *total_size_out) {
  const bool rv = mock().actualCall(__func__)
                        .withOutputParameter("total_size_out", total_size_out)
                        .returnBoolValueOrDefault(true);
  if (rv && total_size_out) {
    *total_size_out = g_mock_memfault_coredump_total_size;
  }
  return rv;
}

void memfault_platform_coredump_storage_clear(void) {
  mock().actualCall(__func__);
}

const sMemfaultDataSourceImpl g_memfault_coredump_data_source = {
  .has_more_msgs_cb = memfault_coredump_has_valid_coredump,
  .read_msg_cb = memfault_platform_coredump_storage_read,
  .mark_msg_read_cb = memfault_platform_coredump_storage_clear,
};
