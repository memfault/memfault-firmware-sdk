//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

// clang-format off
#include "memfault/core/platform/core.h"

#include <stdio.h>
#include <sys/types.h>
#if defined(CONFIG_MEMFAULT_REBOOT_REASON_GET_HWINFO)
#include MEMFAULT_ZEPHYR_INCLUDE(drivers/hwinfo.h)
#endif
#include MEMFAULT_ZEPHYR_INCLUDE(init.h)
#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)
#include MEMFAULT_ZEPHYR_INCLUDE(logging/log_ctrl.h)
#include <soc.h>
#if defined(CONFIG_MEMFAULT_DATETIME_TIMESTAMP_EVENT_CALLBACK)
#include <date_time.h>
#include "memfault/ports/ncs/date_time_callback.h"
#endif

#include "memfault/components.h"
#include "memfault/ports/reboot_reason.h"
#include "memfault/ports/zephyr/core.h"
#include "memfault/ports/zephyr/log_backend.h"
#include MEMFAULT_ZEPHYR_INCLUDE(sys/__assert.h)
#include "memfault/ports/zephyr/version.h"
// clang-format on

#if CONFIG_MEMFAULT_METRICS
  #include "memfault/metrics/metrics.h"
#endif

static const sMemfaultEventStorageImpl *s_memfault_event_storage;

#if CONFIG_MEMFAULT_CACHE_FAULT_REGS
// Zephy's z_arm_fault() function consumes and clears
// the SCB->CFSR register so we must wrap their function
// so we can preserve the pristine fault register values.
void __wrap_z_arm_fault(uint32_t msp, uint32_t psp, uint32_t exc_return,
                        _callee_saved_t *callee_regs) {
  #if defined(CONFIG_MEMFAULT_LOGGING_ENABLE) && \
    (MEMFAULT_ZEPHYR_VERSION_GT(3, 1) || defined(CONFIG_LOG2))
  // Trigger a LOG_PANIC() early to flush any buffered logs, then disable the
  // Memfault log backend to prevent any further logs from being captured
  // (primarily the Zephyr fault logs, which can fill up the Memfault log
  // buffer). Note that this approach won't work if the user has Logs enabled
  // but CONFIG_MEMFAULT_CACHE_FAULT_REGS=n, and the fault messages will end up
  // in the log buffer. That should be an unusual configuration, since the fault
  // register capture disable is a very small size optimization, and logs are
  // likely not used on devices with space constraints.
  LOG_PANIC();
    #if !defined(CONFIG_LOG_MODE_MINIMAL)
  memfault_zephyr_log_backend_disable();
    #endif
  #endif

  memfault_coredump_cache_fault_regs();

  // Now let the Zephyr fault handler complete as normal.
  void __real_z_arm_fault(uint32_t msp, uint32_t psp, uint32_t exc_return,
                          _callee_saved_t *callee_regs);
  __real_z_arm_fault(msp, psp, exc_return, callee_regs);
}
#endif

uint64_t memfault_platform_get_time_since_boot_ms(void) {
  return k_uptime_get();
}

//! Provide a strong implementation of assert_post_action for Zephyr's built-in
//! __ASSERT() macro.
#if CONFIG_MEMFAULT_CATCH_ZEPHYR_ASSERT
  #ifdef CONFIG_ASSERT_NO_FILE_INFO
void assert_post_action(void)
  #else
void assert_post_action(const char *file, unsigned int line)
  #endif
{
  #ifndef CONFIG_ASSERT_NO_FILE_INFO
  ARG_UNUSED(file);
  ARG_UNUSED(line);
  #endif
  MEMFAULT_ASSERT(0);
}
#endif

#if defined(CONFIG_MEMFAULT_METRICS_BOOT_TIME)
  #if defined(CONFIG_BOOTARGS)
extern int __real_main(int, char **);
int __wrap_main(int argc, char **argv);
int __wrap_main(int argc, char **argv) {
  #else
extern int __real_main(void);
int __wrap_main(void);
int __wrap_main(void) {
  #endif
  // Record the boot time once on startup, when main() is called.
  MEMFAULT_METRIC_SET_UNSIGNED(boot_time_ms, k_uptime_get());

  #if defined(CONFIG_BOOTARGS)
  return __real_main(argc, argv);
  #else
  return __real_main();
  #endif
}
#endif  // CONFIG_MEMFAULT_METRICS_BOOT_TIME

// On boot-up, log out any information collected as to why the
// reset took place

MEMFAULT_PUT_IN_SECTION(CONFIG_MEMFAULT_REBOOT_TRACKING_REGION) static uint8_t
  s_reboot_tracking[MEMFAULT_REBOOT_TRACKING_REGION_SIZE];

static uint8_t s_event_storage[CONFIG_MEMFAULT_EVENT_STORAGE_SIZE];

#if defined(CONFIG_MEMFAULT_REBOOT_REASON_GET_HWINFO)
static eMemfaultRebootReason prv_zephyr_to_memfault_reboot_reason(uint32_t reset_reason_reg) {
  // Some hwinfo device implementations will use a value of 0 (unset) to
  // implicitly specify a power on reset, for example the nRF52840:
  // https://docs.nordicsemi.com/bundle/ps_nrf52840/page/power.html#register.RESETREAS
  //
  // Only use this fallback when the hwinfo implementation does not explicitly
  // support a power on reset cause.
  uint32_t supported_reset_reasons;
  if ((reset_reason_reg == 0) &&
      (hwinfo_get_supported_reset_cause(&supported_reset_reasons) == 0) &&
      !(supported_reset_reasons & RESET_POR)) {
    return kMfltRebootReason_PowerOnReset;
  }

  // Map the Zephyr HWINFO reset reason to a Memfault reset reason. The order is
  // important- the first bit match will be used.
  //
  // We know that the Zephyr bits + Memfault reboot codes all fit within 16
  // bits, so to cut the table size in half, use uint16_t. If either one grows
  // beyond 16 bits, this will need to be updated.
  //
  // A further optimization (that saves a few more bytes) is to omit the
  // bitmask, since they're consecutive in the current zephyr implementation.
  // That forces us to keep the reset reason priority in the zephyr bit order.
  eMemfaultRebootReason reset_reason = kMfltRebootReason_Unknown;
  const struct hwinfo_bit_to_memfault_reset_reason {
    uint16_t hwinfo_bit;
    uint16_t memfault_reason;
  } s_hwinfo_to_memfault[] = {
    { RESET_PIN, kMfltRebootReason_PinReset },
    { RESET_SOFTWARE, kMfltRebootReason_SoftwareReset },
    { RESET_BROWNOUT, kMfltRebootReason_BrownOutReset },
    { RESET_POR, kMfltRebootReason_PowerOnReset },
    { RESET_WATCHDOG, kMfltRebootReason_HardwareWatchdog },
    { RESET_DEBUG, kMfltRebootReason_DebuggerHalted },
    { RESET_SECURITY, kMfltRebootReason_SecurityViolation },
    { RESET_LOW_POWER_WAKE, kMfltRebootReason_LowPower },
    { RESET_CPU_LOCKUP, kMfltRebootReason_Lockup },
    { RESET_PARITY, kMfltRebootReason_ParityError },
    { RESET_PLL, kMfltRebootReason_ClockFailure },
    { RESET_CLOCK, kMfltRebootReason_ClockFailure },
    { RESET_HARDWARE, kMfltRebootReason_Hardware },
    { RESET_USER, kMfltRebootReason_UserReset },
    { RESET_TEMPERATURE, kMfltRebootReason_Temperature },
  };

  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(s_hwinfo_to_memfault); i++) {
    if (reset_reason_reg & (uint32_t)s_hwinfo_to_memfault[i].hwinfo_bit) {
      reset_reason = (eMemfaultRebootReason)s_hwinfo_to_memfault[i].memfault_reason;
      break;
    }
  }

  return reset_reason;
}
#endif  // CONFIG_MEMFAULT_REBOOT_REASON_GET_HWINFO

#if !defined(CONFIG_MEMFAULT_REBOOT_REASON_GET_CUSTOM)
MEMFAULT_WEAK void memfault_reboot_reason_get(sResetBootupInfo *info) {
  eMemfaultRebootReason reset_reason = kMfltRebootReason_Unknown;
  uint32_t reset_reason_reg = 0;
  #if defined(CONFIG_MEMFAULT_REBOOT_REASON_GET_HWINFO)
  int rv = hwinfo_get_reset_cause(&reset_reason_reg);

  if (rv == 0) {
    reset_reason = prv_zephyr_to_memfault_reboot_reason(reset_reason_reg);

    #if defined(CONFIG_MEMFAULT_ENABLE_REBOOT_DIAG_DUMP)
    // Print out the reset reason to the console.
    const char *reset_reason_str = "Unknown";

    static const struct {
      eMemfaultRebootReason reason;
      const char *str;
    } s_reset_reasons[] = {
      { kMfltRebootReason_PinReset, "Pin Reset" },
      { kMfltRebootReason_SoftwareReset, "Software Reset" },
      { kMfltRebootReason_BrownOutReset, "Brownout Reset" },
      { kMfltRebootReason_PowerOnReset, "Power On Reset" },
      { kMfltRebootReason_HardwareWatchdog, "Hardware Watchdog" },
      { kMfltRebootReason_DebuggerHalted, "Debugger Halted" },
      { kMfltRebootReason_SecurityViolation, "Security Violation" },
      { kMfltRebootReason_LowPower, "Low Power" },
      { kMfltRebootReason_Lockup, "Lockup" },
      { kMfltRebootReason_ParityError, "Parity Error" },
      { kMfltRebootReason_ClockFailure, "Clock Failure" },
      { kMfltRebootReason_Hardware, "Hardware" },
      { kMfltRebootReason_UserReset, "User Reset" },
      { kMfltRebootReason_Temperature, "Temperature" },
      { kMfltRebootReason_Unknown, "Unknown" },
    };

    for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(s_reset_reasons); i++) {
      if (s_reset_reasons[i].reason == reset_reason) {
        reset_reason_str = s_reset_reasons[i].str;
        break;
      }
    }

    printk("Reset Cause: %s / 0x%04x\n", reset_reason_str, reset_reason_reg);
    #endif  // CONFIG_MEMFAULT_ENABLE_REBOOT_DIAG_DUMP
  }

    #if defined(CONFIG_MEMFAULT_CLEAR_RESET_REG)
  (void)hwinfo_clear_reset_cause();
    #endif

  #endif  // CONFIG_MEMFAULT_REBOOT_REASON_GET_HWINFO

  *info = (sResetBootupInfo){
    .reset_reason = reset_reason,
    .reset_reason_reg = reset_reason_reg,
  };
}
#endif  // !CONFIG_MEMFAULT_REBOOT_REASON_GET_CUSTOM

void memfault_zephyr_collect_reset_info(void) {
  memfault_reboot_tracking_collect_reset_info(s_memfault_event_storage);
}

// This can be overridden by the application to set a custom device ID
MEMFAULT_WEAK const char *memfault_zephyr_get_device_id(void) {
  uint8_t dev_id[16] = { 0 };
  static char dev_id_str[sizeof(dev_id) * 2 + 1];
  static const char *dev_str = "UNKNOWN";

// Obtain the device id
#if defined(CONFIG_MEMFAULT_REBOOT_REASON_GET_HWINFO)
  ssize_t length = hwinfo_get_device_id(dev_id, sizeof(dev_id));
#else
  ssize_t length = 0;
#endif

  // If hwinfo_get_device_id() fails or isn't enabled, use a fallback string
  if (length <= 0) {
#if defined(CONFIG_SOC)
    dev_str = CONFIG_SOC "-testserial";
#else
    dev_str = "testserial";
#endif
    length = strlen(dev_str);
  } else {
    // Render the obtained serial number in hexadecimal representation
    for (size_t i = 0; i < length; i++) {
      (void)snprintf(&dev_id_str[i * 2], sizeof(dev_id_str), "%02x", dev_id[i]);
    }
    dev_str = dev_id_str;
  }

  return dev_str;
}

MEMFAULT_WEAK void memfault_platform_get_device_info(sMemfaultDeviceInfo *info) {
  *info = (sMemfaultDeviceInfo){
    .device_serial = memfault_zephyr_get_device_id(),
    .software_type = CONFIG_MEMFAULT_BUILTIN_DEVICE_INFO_SOFTWARE_TYPE,
    .software_version = CONFIG_MEMFAULT_BUILTIN_DEVICE_INFO_SOFTWARE_VERSION,
    .hardware_version = CONFIG_MEMFAULT_BUILTIN_DEVICE_INFO_HARDWARE_VERSION,
  };
}

// Note: the function signature has changed here across zephyr releases
// "struct device *dev" -> "const struct device *dev"
//
// Since we don't use the arguments we match anything with () to avoid
// compiler warnings and share the same bootup logic
static int prv_boot_memfault() {
  sResetBootupInfo reset_info = { 0 };
  memfault_reboot_reason_get(&reset_info);
  memfault_reboot_tracking_boot(s_reboot_tracking, &reset_info);

  s_memfault_event_storage = memfault_events_storage_boot(s_event_storage, sizeof(s_event_storage));
#if defined(CONFIG_MEMFAULT_RECORD_REBOOT_ON_SYSTEM_INIT)
  memfault_zephyr_collect_reset_info();
#endif
  memfault_trace_event_boot(s_memfault_event_storage);

#if CONFIG_MEMFAULT_METRICS
  sMemfaultMetricBootInfo boot_info = {
    .unexpected_reboot_count = memfault_reboot_tracking_get_crash_count(),
  };
  memfault_metrics_boot(s_memfault_event_storage, &boot_info);
#endif

#if defined(CONFIG_MEMFAULT_BATTERY_METRICS_BOOT_ON_SYSTEM_INIT)
  memfault_metrics_battery_boot();
#endif

#if defined(CONFIG_MEMFAULT_DATETIME_TIMESTAMP_EVENT_CALLBACK)
  // Register a callback to get the current time from the Zephyr Date-Time library
  date_time_register_handler(memfault_zephyr_date_time_evt_handler);
#endif

  memfault_build_info_dump();
  return 0;
}

#if CONFIG_MEMFAULT_HEAP_STATS && CONFIG_HEAP_MEM_POOL_SIZE > 0
extern void *__real_k_malloc(size_t size);
extern void __real_k_free(void *ptr);

void *__wrap_k_malloc(size_t size) {
  void *ptr = __real_k_malloc(size);

  // Only call into heap stats from non-ISR context
  // Heap stats requires holding a lock
  if (!k_is_in_isr()) {
    MEMFAULT_HEAP_STATS_MALLOC(ptr, size);
  }

  return ptr;
}
void __wrap_k_free(void *ptr) {
  // Only call into heap stats from non-ISR context
  // Heap stats requires holding a lock
  if (!k_is_in_isr()) {
    MEMFAULT_HEAP_STATS_FREE(ptr);
  }
  __real_k_free(ptr);
}
#endif

SYS_INIT(prv_boot_memfault,
#if CONFIG_MEMFAULT_INIT_LEVEL_POST_KERNEL
         POST_KERNEL,
#else
         APPLICATION,
#endif
         CONFIG_MEMFAULT_INIT_PRIORITY);
