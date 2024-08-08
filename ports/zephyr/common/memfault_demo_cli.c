//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Adds a basic set of commands for interacting with Memfault SDK

// clang-format off
#include MEMFAULT_ZEPHYR_INCLUDE(shell/shell.h)
#include <stdlib.h>

#include "memfault/components.h"
#include "memfault/ports/zephyr/http.h"
#include "memfault/ports/zephyr/fota.h"
#include "zephyr_release_specific_headers.h"
// clang-format on

static int prv_clear_core_cmd(const struct shell *shell, size_t argc, char **argv) {
  return memfault_demo_cli_cmd_clear_core(argc, argv);
}

static int prv_get_core_cmd(const struct shell *shell, size_t argc, char **argv) {
  return memfault_demo_cli_cmd_get_core(argc, argv);
}

static int prv_test_log(const struct shell *shell, size_t argc, char **argv) {
  return memfault_demo_cli_cmd_test_log(argc, argv);
}

static int prv_trigger_logs(const struct shell *shell, size_t argc, char **argv) {
  return memfault_demo_cli_cmd_trigger_logs(argc, argv);
}

static int prv_get_device_info(const struct shell *shell, size_t argc, char **argv) {
  return memfault_demo_cli_cmd_get_device_info(argc, argv);
}

//! Route the 'export' command to output via printk, so we don't drop messages
//! from logging a big burst.
void memfault_data_export_base64_encoded_chunk(const char *base64_chunk) {
  printk("%s\n", base64_chunk);
}

static int prv_chunk_data_export(const struct shell *shell, size_t argc, char **argv) {
#if defined(CONFIG_LOG_PRINTK) && !defined(CONFIG_LOG_MODE_IMMEDIATE)
  // printk is configured to pass through the deferred logging subsystem,
  // which can result in dropped Memfault chunk messages
  MEMFAULT_LOG_WARN("CONFIG_LOG_PRINTK=y and CONFIG_LOG_MODE_IMMEDIATE=n can result in dropped "
                    "'mflt export' messages");
#endif
  memfault_data_export_dump_chunks();
  return 0;
}

static int prv_example_trace_event_capture(const struct shell *shell, size_t argc, char **argv) {
  // For more information on user-defined error reasons, see
  // the MEMFAULT_TRACE_REASON_DEFINE macro in trace_reason_user.h .
  MEMFAULT_TRACE_EVENT_WITH_LOG(MemfaultCli_Test, "Num args: %d", (int)argc);
  MEMFAULT_LOG_DEBUG("Trace Event Generated!");
  return 0;
}

static int prv_post_data(const struct shell *shell, size_t argc, char **argv) {
  // For more information on user-defined error reasons, see
  // the MEMFAULT_TRACE_REASON_DEFINE macro in trace_reason_user.h .
#if defined(CONFIG_MEMFAULT_HTTP_ENABLE)
  MEMFAULT_LOG_INFO("Posting Memfault Data");
  ssize_t rv = memfault_zephyr_port_post_data_return_size();

  if (rv < 0) {
    MEMFAULT_LOG_ERROR("Failed to post data, err=%d", rv);
  } else if (rv == 0) {
    MEMFAULT_LOG_INFO("Done: no data to send");
  } else {
    MEMFAULT_LOG_INFO("Data posted successfully, %d bytes sent", rv);
  }
  return (rv < 0) ? rv : 0;
#else
  shell_print(shell, "CONFIG_MEMFAULT_HTTP_ENABLE not enabled");
  return 0;
#endif
}

static int prv_get_latest_url_cmd(const struct shell *shell, size_t argc, char **argv) {
#if defined(CONFIG_MEMFAULT_HTTP_ENABLE)
  char *url = NULL;
  int rv = memfault_zephyr_port_get_download_url(&url);
  if (rv < 0) {
    MEMFAULT_LOG_ERROR("Unable to fetch OTA url, rv=%d", rv);
    return rv;
  } else if (rv == 0) {
    MEMFAULT_LOG_INFO("Up to date!");
    return 0;
  }

  shell_print(shell, "Download URL: '%s'", url);
  rv = memfault_zephyr_port_release_download_url(&url);
  return rv;
#else
  shell_print(shell, "CONFIG_MEMFAULT_HTTP_ENABLE not enabled");
  return 0;
#endif /* CONFIG_MEMFAULT_HTTP_ENABLE */
}

static int prv_check_and_fetch_ota_payload_cmd(const struct shell *shell, size_t argc,
                                               char **argv) {
#if defined(CONFIG_MEMFAULT_NRF_CONNECT_SDK)
  // The nRF Connect SDK comes with a download client module that can be used to
  // perform an actual e2e OTA so use that instead and don't link this code in at all!
  shell_print(shell,
              "Use 'mflt_nrf fota' instead. See https://mflt.io/nrf-fota-setup for more details");
#elif defined(CONFIG_MEMFAULT_HTTP_ENABLE)
  return memfault_zephyr_fota_start();
#else
  shell_print(shell, "CONFIG_MEMFAULT_HTTP_ENABLE not enabled");
#endif
  return 0;
}

static int prv_coredump_size(const struct shell *shell, size_t argc, char **argv) {
  (void)argc, (void)argv;

  size_t capacity, total_size;
  memfault_coredump_size_and_storage_capacity(&total_size, &capacity);

  shell_print(shell, "coredump storage capacity: %zuB", capacity);
  shell_print(shell, "coredump size required: %zuB", total_size);

  return 0;
}

static int prv_trigger_heartbeat(const struct shell *shell, size_t argc, char **argv) {
#if CONFIG_MEMFAULT_METRICS
  shell_print(shell, "Triggering Heartbeat");
  memfault_metrics_heartbeat_debug_trigger();
  return 0;
#else
  shell_print(shell, "CONFIG_MEMFAULT_METRICS not enabled");
  return 0;
#endif
}

static int prv_metrics_dump(const struct shell *shell, size_t argc, char **argv) {
#if defined(CONFIG_MEMFAULT_METRICS)
  if (argc < 2) {
    shell_print(shell, "Enter 'heartbeat' or 'sessions'");
    return 0;
  }

  if (!strcmp(argv[1], "sessions")) {
    memfault_metrics_all_sessions_debug_print();
  } else if (!strcmp(argv[1], "heartbeat")) {
    memfault_metrics_heartbeat_debug_print();
  } else {
    shell_print(shell, "Unknown option. Enter 'heartbeat' or 'sessions'");
    return 0;
  }
#else
  shell_print(shell, "CONFIG_MEMFAULT_METRICS not enabled");
#endif
  return 0;
}

static int prv_test_reboot(const struct shell *shell, size_t argc, char **argv) {
  memfault_reboot_tracking_mark_reset_imminent(kMfltRebootReason_UserReset, NULL);
  memfault_platform_reboot();
  return 0;  // should be unreachable
}

static int prv_memfault_assert_example(const struct shell *shell, size_t argc, char **argv) {
  memfault_demo_cli_cmd_assert(argc, argv);
  return -1;
}

static int prv_hang_example(const struct shell *shell, size_t argc, char **argv) {
#if !CONFIG_WATCHDOG
  shell_print(shell, "No watchdog configured, this will hang forever");
#else
  MEMFAULT_LOG_DEBUG("Hanging system and waiting for watchdog!");
#endif
  while (1) { }
  return -1;
}

#if defined(CONFIG_ARM)

static int prv_busfault_example(const struct shell *shell, size_t argc, char **argv) {
  #if defined(CONFIG_MEMFAULT_NRF_CONNECT_SDK) && defined(CONFIG_BUILD_WITH_TFM) && \
    !defined(CONFIG_TFM_ALLOW_NON_SECURE_FAULT_HANDLING)
  // Starting in NCS v2.6.0, enabling CONFIG_TFM_ALLOW_NON_SECURE_FAULT_HANDLING will cause
  // Memfault's fault handlers to be invoked for BusFaults originating from non-secure code.
  shell_print(shell, "CONFIG_TFM_ALLOW_NON_SECURE_FAULT_HANDLING is disabled or undefined, no "
                     "coredump will be collected");

  // Allow the shell output buffer to be flushed before the crash
  k_sleep(K_MSEC(100));
  #endif
  //! Note: The Zephyr fault handler dereferences the pc which triggers a fault
  //! if the pc itself is from a bad pointer:
  //! https://github.com/zephyrproject-rtos/zephyr/blob/f400c94/arch/arm/core/aarch32/cortex_m/fault.c#L664
  //!
  //! We set the BFHFNMIGN bit to prevent a lockup from happening due to de-referencing the bad PC
  //! which generated the fault in the first place
  volatile uint32_t *ccr = (uint32_t *)0xE000ED14;
  *ccr |= 0x1 << 8;

  memfault_demo_cli_cmd_busfault(argc, argv);
  return -1;
}

static int prv_hardfault_example(const struct shell *shell, size_t argc, char **argv) {
  #if defined(CONFIG_MEMFAULT_NRF_CONNECT_SDK) && defined(CONFIG_BUILD_WITH_TFM)
  // Memfault's fault handlers are not invoked during the handling of HardFaults in NCS when using
  // TF-M. CONFIG_TFM_ALLOW_NON_SECURE_FAULT_HANDLING=y will only invoke Memfault's fault handlers
  // for BusFaults and SecureFaults originating from non-secure code, not Hardfaults.
  shell_print(shell, "HardFaults are handled by TF-M, no coredump will be collected");

  // Allow the shell output buffer to be flushed before the crash
  k_sleep(K_MSEC(100));
  #endif
  memfault_demo_cli_cmd_hardfault(argc, argv);
  return -1;
}

static int prv_usagefault_example(const struct shell *shell, size_t argc, char **argv) {
  memfault_demo_cli_cmd_usagefault(argc, argv);
  return -1;
}

static int prv_memmanage_example(const struct shell *shell, size_t argc, char **argv) {
  memfault_demo_cli_cmd_memmanage(argc, argv);
  return -1;
}

#endif  // CONFIG_ARM

static int prv_zephyr_assert_example(const struct shell *shell, size_t argc, char **argv) {
#if defined(CONFIG_ASSERT)
  // Fire off a last-ditch log message to show how logs are flushed prior to
  // crash, in the case of deferred logging mode
  MEMFAULT_LOG_ERROR("About to crash in %s!", __func__);
  __ASSERT(0, "test __ASSERT");
#else
  shell_print(shell, "CONFIG_ASSERT was disabled in the build, this command has no effect");
#endif
  return 0;
}

static int prv_zephyr_load_32bit_address(const struct shell *shell, size_t argc, char **argv) {
  return memfault_demo_cli_loadaddr(argc, argv);
}

static int prv_cli_cmd_double_free(const struct shell *shell, size_t argc, char **argv) {
  (void)shell;
  (void)argc;
  (void)argv;

#if !CONFIG_MEMFAULT_HEAP_STATS
  shell_print(
    shell,
    "CONFIG_MEMFAULT_HEAP_STATS was disabled in the build, this command will have no effect");
#else
  uint8_t *ptr = k_malloc(100);
  k_free(ptr);
  k_free(ptr);

  shell_print(shell, "Double free should have crashed the app! 🤯 Make sure CONFIG_ASSERT=y");
#endif

  return -1;
}

static int prv_bad_ptr_deref_example(const struct shell *shell, size_t argc, char **argv) {
  volatile uint32_t *bad_ptr = (void *)0x1;
  *bad_ptr = 0xbadcafe;
  return -1;
}

static void prv_crash_timer_handler(struct k_timer *dummy) {
  volatile uint32_t *bad_ptr = (void *)0x1;
  *bad_ptr = 0xbadcafe;
}

K_TIMER_DEFINE(s_isr_crash_timer, prv_crash_timer_handler, NULL);

static int prv_timer_isr_crash_example(const struct shell *shell, size_t argc, char **argv) {
  k_timer_start(&s_isr_crash_timer, K_MSEC(10), K_MSEC(10));
  return 0;
}

#if defined(CONFIG_MEMFAULT_SHELL_SELF_TEST)
static int prv_self_test(MEMFAULT_UNUSED const struct shell *shell, size_t argc, char **argv) {
  return memfault_demo_cli_cmd_self_test(argc, argv);
}
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(
  sub_memfault_crash_cmds,
//! different crash types that should result in a coredump being collected

#if defined(CONFIG_ARM)
  SHELL_CMD(busfault, NULL, "trigger a busfault", prv_busfault_example),
  SHELL_CMD(hardfault, NULL, "trigger a hardfault", prv_hardfault_example),
  SHELL_CMD(memmanage, NULL, "trigger a memory management fault", prv_memmanage_example),
  SHELL_CMD(usagefault, NULL, "trigger a usage fault", prv_usagefault_example),
#endif  // CONFIG_ARM

  SHELL_CMD(hang, NULL, "trigger a hang", prv_hang_example),
  SHELL_CMD(zassert, NULL, "trigger a zephyr assert", prv_zephyr_assert_example),
  SHELL_CMD(assert, NULL, "trigger memfault assert", prv_memfault_assert_example),
  SHELL_CMD(loadaddr, NULL, "test a 32 bit load from an address", prv_zephyr_load_32bit_address),
  SHELL_CMD(double_free, NULL, "trigger a double free error", prv_cli_cmd_double_free),
  SHELL_CMD(badptr, NULL, "trigger fault via store to a bad address", prv_bad_ptr_deref_example),
  SHELL_CMD(isr_badptr, NULL, "trigger fault via store to a bad address from an ISR",
            prv_timer_isr_crash_example),

  //! user initiated reboot
  SHELL_CMD(reboot, NULL, "trigger a reboot and record it using memfault", prv_test_reboot),

  //! memfault data source test commands
  SHELL_CMD(heartbeat, NULL, "trigger an immediate capture of all heartbeat metrics",
            prv_trigger_heartbeat),
  SHELL_CMD(log_capture, NULL, "trigger capture of current log buffer contents", prv_trigger_logs),
  SHELL_CMD(logs, NULL, "writes test logs to log buffer", prv_test_log),
  SHELL_CMD(trace, NULL, "capture an example trace event", prv_example_trace_event_capture),
#if defined(CONFIG_MEMFAULT_SHELL_SELF_TEST)
  SHELL_CMD(self, NULL, "test Memfault components on-device", prv_self_test),
#endif

  SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_STATIC_SUBCMD_SET_CREATE(
  sub_memfault_cmds, SHELL_CMD(clear_core, NULL, "clear coredump collected", prv_clear_core_cmd),
  SHELL_CMD(export, NULL,
            "dump chunks collected by Memfault SDK using https://mflt.io/chunk-data-export",
            prv_chunk_data_export),
  SHELL_CMD(get_core, NULL, "check if coredump is stored and present", prv_get_core_cmd),
  SHELL_CMD(get_device_info, NULL, "display device information", prv_get_device_info),
  SHELL_CMD(get_latest_url, NULL, "gets latest release URL", prv_get_latest_url_cmd),
  SHELL_CMD(get_latest_release, NULL, "performs an OTA update using Memfault client",
            prv_check_and_fetch_ota_payload_cmd),
  SHELL_CMD(coredump_size, NULL, "print coredump computed size and storage capacity",
            prv_coredump_size),
  SHELL_CMD(metrics_dump, NULL, "dump current heartbeat or session metrics", prv_metrics_dump),
  SHELL_CMD(post_chunks, NULL, "Post Memfault data to cloud", prv_post_data),
  SHELL_CMD(test, &sub_memfault_crash_cmds,
            "commands to verify memfault data collection (https://mflt.io/mcu-test-commands)",
            NULL),
  SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(mflt, &sub_memfault_cmds, "Memfault Test Commands", NULL);
