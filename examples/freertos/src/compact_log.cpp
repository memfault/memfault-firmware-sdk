//! @file
//!
//! Small example showcasing the use of compact logs from C++.

#include "compact_log.h"

#include <inttypes.h>

#include "memfault/components.h"

void compact_log_cpp_example(void) {
#if MEMFAULT_COMPACT_LOG_ENABLE
  MEMFAULT_COMPACT_LOG_SAVE(kMemfaultPlatformLogLevel_Info,
                            "This is a compact log example from c++ "
                            // clang-format off
    "%d"  " %" PRIu64    " %f"  " %f"   " %s"   " %s"             " %s"           " %s"                    " %s",
    1234, (uint64_t)0x7, 1.0f,  2.0,    "1212", (uint8_t*)"1212", (int8_t*)"1212", (const uint8_t*)"1212", (const int8_t*)"1212"
//  ^int  ^uint64_t      ^float ^double ^char*  ^unsigned char*   ^signed char*    ^const unsigned char*   ^const signed char*
                            // clang-format on
  );
#endif
}
