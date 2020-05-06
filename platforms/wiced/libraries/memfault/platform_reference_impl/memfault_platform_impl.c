//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//! Reference implementation of the memfault platform header for the WICED platform

#include "platform_assert.h" // import WICED_TRIGGER_BREAKPOINT
#include "platform_cmsis.h" // import NVIC_SystemReset & CoreDebug

#include "memfault/core/compiler.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/errors.h"
#include "memfault/core/platform/core.h"

#include "memfault/http/root_certs.h"
#include "memfault_platform_wiced.h"

#include "wiced_tls.h"
#include "wiced_result.h"

// return different error codes from each exit point so it's easier to determine what went wrong
typedef enum {
  kMemfaultPlatformBoot_DeviceInfoBootFailed = -1,
  kMemfaultPlatformBoot_CoredumpBootFailed = -2,
} eMemfaultPlatformBoot;

int memfault_platform_boot(void) {
  if (!memfault_platform_device_info_boot()) {
    MEMFAULT_LOG_ERROR("memfault_platform_device_info_boot() failed");
    return kMemfaultPlatformBoot_DeviceInfoBootFailed;
  }
  if (!memfault_platform_coredump_boot()) {
    MEMFAULT_LOG_ERROR("memfault_platform_coredump_boot() failed");
    return kMemfaultPlatformBoot_CoredumpBootFailed;
  }

  const wiced_result_t result = wiced_tls_init_root_ca_certificates(
      MEMFAULT_ROOT_CERTS_PEM, sizeof(MEMFAULT_ROOT_CERTS_PEM) - 1);
  if (result != WICED_SUCCESS) {
    MEMFAULT_LOG_ERROR("wiced_tls_init_root_ca_certificates() failed: %u", result);
    return MEMFAULT_PLATFORM_SPECIFIC_ERROR(result);
  }

  return 0;
}

MEMFAULT_NORETURN void memfault_platform_reboot(void) {
  memfault_platform_halt_if_debugging();

  NVIC_SystemReset();
  MEMFAULT_UNREACHABLE;
}
