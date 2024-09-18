//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! See header for more details

#include <stdint.h>
#include <string.h>

#include "memfault/config.h"
#include "memfault/core/build_info.h"
#include "memfault_build_id_private.h"

// OS version information
#if defined(MEMFAULT_OS_VERSION_NAME) ^ defined(MEMFAULT_OS_VERSION_STRING)
  #error "MEMFAULT_OS_VERSION_NAME and MEMFAULT_OS_VERSION_STRING must be both defined or undefined"
#elif !(defined(MEMFAULT_OS_VERSION_NAME) && defined(MEMFAULT_OS_VERSION_STRING))
  #if defined(__ZEPHYR__)
    // Either Zephyr or NCS. We can then rely on the GNU __has_include extension to
    // be available.
    #if __has_include("ncs_version.h")
      #include "ncs_version.h"
      #define MEMFAULT_OS_VERSION_NAME "ncs"
      #define MEMFAULT_OS_VERSION_STRING NCS_VERSION_STRING
    #else
      // This is Zephyr's version.h file, since Memfault's is namespaced as
      // "memfault/version.h".
      #include <version.h>
      #define MEMFAULT_OS_VERSION_NAME "zephyr"
      #define MEMFAULT_OS_VERSION_STRING KERNEL_VERSION_STRING
    #endif /* __has_include("ncs_version.h") */

  #elif defined(ESP_PLATFORM)
    // ESP-IDF, use the command-line defined IDF_VER string
    #define MEMFAULT_OS_VERSION_NAME "esp-idf"
    #define MEMFAULT_OS_VERSION_STRING IDF_VER
  #else
    // No OS version information available
    #define MEMFAULT_OS_VERSION_NAME ""
    #define MEMFAULT_OS_VERSION_STRING ""
  #endif /* defined(__ZEPHYR__) */
#endif   /* defined(MEMFAULT_OS_VERSION_NAME) ^ defined(MEMFAULT_OS_VERSION_STRING) \
          */

#if MEMFAULT_USE_GNU_BUILD_ID

// Note: This variable is emitted by the linker script
extern uint8_t MEMFAULT_GNU_BUILD_ID_SYMBOL[];

MEMFAULT_BUILD_ID_QUALIFIER sMemfaultBuildIdStorage g_memfault_build_id = {
  .type = kMemfaultBuildIdType_GnuBuildIdSha1,
  .len = sizeof(sMemfaultElfNoteSection),
  .short_len = MEMFAULT_EVENT_INCLUDED_BUILD_ID_SIZE_BYTES,
  .storage = MEMFAULT_GNU_BUILD_ID_SYMBOL,
  .sdk_version = MEMFAULT_SDK_VERSION,
  .os_version = {
    .name = MEMFAULT_OS_VERSION_NAME,
    .version = MEMFAULT_OS_VERSION_STRING,
  },
};
#else

// NOTE: We start the array with a 0x1, so the compiler will never place the variable in bss
MEMFAULT_BUILD_ID_QUALIFIER uint8_t g_memfault_sdk_derived_build_id[MEMFAULT_BUILD_ID_LEN] = {
  0x1,
};

MEMFAULT_BUILD_ID_QUALIFIER sMemfaultBuildIdStorage g_memfault_build_id = {
  .type = kMemfaultBuildIdType_None,
  .len = sizeof(g_memfault_sdk_derived_build_id),
  .short_len = MEMFAULT_EVENT_INCLUDED_BUILD_ID_SIZE_BYTES,
  .storage = g_memfault_sdk_derived_build_id,
  .sdk_version = MEMFAULT_SDK_VERSION,
  .os_version = {
    .name = MEMFAULT_OS_VERSION_NAME,
    .version = MEMFAULT_OS_VERSION_STRING,
  },
};
#endif
