//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! ESP32 CLI implementation for demo application

// TODO: Migrate to "driver/gptimer.h" to fix warning
#include "esp_console.h"
#include "esp_err.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "memfault/esp_port/version.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  #include "driver/gptimer.h"
  #include "esp_private/esp_clk.h"
  #include "soc/timer_periph.h"
#else
  #include "driver/periph_ctrl.h"
  #include "driver/timer.h"
  #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
    #include "esp32/clk.h"
  #else
    #include "esp_clk.h"
  #endif
#endif

#include "memfault/components.h"
#include "memfault/esp_port/cli.h"
#include "memfault/esp_port/http_client.h"

#define TIMER_DIVIDER  (16)  //  Hardware timer clock divider
#define TIMER_SCALE_TICKS_PER_MS(_baseFrequency)  (((_baseFrequency) / TIMER_DIVIDER) / 1000)  // convert counter value to milliseconds

static void IRAM_ATTR prv_recursive_crash(int depth) {
  if (depth == 15) {
    MEMFAULT_ASSERT_RECORD(depth);
  }

  if (depth == 15) {
    // unreachable; this is to silence -Winfinite-recursion
    return;
  }

  // an array to create some stack depth variability
  int dummy_array[depth + 1];
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(dummy_array); i++) {
    dummy_array[i] = (depth << 24) | i;
  }
  dummy_array[depth] = depth + 1;
  prv_recursive_crash(dummy_array[depth]);
}

void prv_check1(const void *buf) {
  MEMFAULT_ASSERT_RECORD(sizeof(buf));
}

void prv_check2(const void *buf) {
  uint8_t buf2[200] = { 0 };
  prv_check1(buf2);
}

void prv_check3(const void *buf) {
  uint8_t buf3[300] = { 0 };
  prv_check2(buf3);
}

void prv_check4(void) {
  uint8_t buf4[400] = { 0 };
  prv_check3(buf4);
}

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
static bool IRAM_ATTR prv_timer_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata,
                                   void *user_ctx) {
  (void)timer, (void)edata, (void)user_ctx;
  // Crash from ISR:
  ESP_ERROR_CHECK(-1);
  return true;
}

static gptimer_handle_t s_gptimer = NULL;
static void prv_timer_init(void) {
  gptimer_config_t timer_config = {
    .clk_src = GPTIMER_CLK_SRC_DEFAULT,
    .direction = GPTIMER_COUNT_UP,
    .resolution_hz = 1000000,  // 1MHz, 1 tick=ums
  };
  ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &s_gptimer));

  gptimer_event_callbacks_t cbs = {
    .on_alarm = prv_timer_cb,
  };
  ESP_ERROR_CHECK(gptimer_register_event_callbacks(s_gptimer, &cbs, NULL));
  ESP_ERROR_CHECK(gptimer_enable(s_gptimer));
}

static void prv_timer_start(uint32_t timer_interval_ms) {
  gptimer_alarm_config_t alarm_config = {
    .alarm_count = timer_interval_ms * 1000,
  };
  ESP_ERROR_CHECK(gptimer_set_alarm_action(s_gptimer, &alarm_config));
  ESP_ERROR_CHECK(gptimer_start(s_gptimer));
}
#else
static void IRAM_ATTR prv_timer_group0_isr(void *para) {
// Always clear the interrupt:
#if CONFIG_IDF_TARGET_ESP32
  #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  TIMERG0.int_clr_timers.t0_int_clr = 1;
  #else
  TIMERG0.int_clr_timers.t0 = 1;
  #endif
#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
  TIMERG0.int_clr_timers.t0_int_clr = 1;
#endif

  // Crash from ISR:
  ESP_ERROR_CHECK(-1);
}

static void prv_timer_init(void)
{
  const timer_config_t config = {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    .clk_src = TIMER_SRC_CLK_DEFAULT,
#endif
    .divider = TIMER_DIVIDER,
    .counter_dir = TIMER_COUNT_UP,
    .counter_en = TIMER_PAUSE,
    .alarm_en = TIMER_ALARM_EN,
    .intr_type = TIMER_INTR_LEVEL,
    .auto_reload = false,
  };
  timer_init(TIMER_GROUP_0, TIMER_0, &config);
  timer_enable_intr(TIMER_GROUP_0, TIMER_0);
  timer_isr_register(TIMER_GROUP_0, TIMER_0, prv_timer_group0_isr, NULL, ESP_INTR_FLAG_IRAM, NULL);
}

static void prv_timer_start(uint32_t timer_interval_ms) {
  uint32_t clock_hz = esp_clk_apb_freq();
  timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0x00000000ULL);
  timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, timer_interval_ms * TIMER_SCALE_TICKS_PER_MS(clock_hz));
  timer_set_alarm(TIMER_GROUP_0, TIMER_0, TIMER_ALARM_EN);
  timer_start(TIMER_GROUP_0, TIMER_0);
}
#endif

static int prv_esp32_assert_example(int argc, char** argv) {
  // default to assert()
  int assert_type =  1;

  if (argc >= 2) {
    assert_type = atoi(argv[1]);
  }

  if (assert_type == 1) {
    assert(0);
  } else if (assert_type == 2) {
    MEMFAULT_ASSERT(0);
  } else if (assert_type == 3) {
    ESP_ERROR_CHECK(-1);
  }

  return 0;
}

static int prv_esp32_crash_example(int argc, char** argv) {
  int crash_type =  0;

  if (argc >= 2) {
    crash_type = atoi(argv[1]);
  }

  if (crash_type == 0) {
    ESP_ERROR_CHECK(10);
  } else if (crash_type == 2) {
    // Crash in timer ISR:
    prv_timer_start(10);
  } else if (crash_type == 3) {
    prv_recursive_crash(0);
  } else if (crash_type == 4) {
    prv_check4();
  }
  return 0;
}

static int prv_esp32_leak_cmd(int argc, char **argv) {
  int leak_size = 0;

  if (argc >= 2) {
    leak_size = atoi(argv[1]);
  }

  if (leak_size == 0) {
    MEMFAULT_LOG_INFO("Usage: leak <size>");
    return 1;
  }

  MEMFAULT_LOG_INFO("Allocating %d bytes", leak_size);
  void *ptr = malloc(leak_size);
  if (ptr == NULL) {
    MEMFAULT_LOG_ERROR("Failed to allocate %d bytes", leak_size);
    return 1;
  }

  MEMFAULT_LOG_INFO("Allocated %d bytes at %p", leak_size, ptr);

  return 0;
}

static int prv_esp32_memfault_heartbeat(int argc, char **argv) {
  memfault_metrics_heartbeat_debug_trigger();
  return 0;
}

static int prv_esp32_memfault_heartbeat_dump(int argc, char** argv) {
  memfault_metrics_heartbeat_debug_print();
  return 0;
}

static bool prv_wifi_connected_check(const char *op) {
  if (memfault_esp_port_wifi_connected()) {
    return true;
  }

  MEMFAULT_LOG_ERROR("Must be connected to WiFi to %s. Use 'join <ssid> <pass>'", op);
  return false;
}

#if MEMFAULT_ESP_HTTP_CLIENT_ENABLE

typedef struct {
  bool perform_ota;
} sMemfaultOtaUserCtx;

static bool prv_handle_ota_upload_available(void *user_ctx) {
  sMemfaultOtaUserCtx *ctx = (sMemfaultOtaUserCtx *)user_ctx;
  MEMFAULT_LOG_DEBUG("OTA Update Available");

  if (ctx->perform_ota) {
    MEMFAULT_LOG_INFO("Starting OTA download ...");
  }
  return ctx->perform_ota;
}

static bool prv_handle_ota_download_complete(void *user_ctx) {
  MEMFAULT_LOG_INFO("OTA Update Complete, Rebooting System");
  esp_restart();
  return true;
}

static int prv_memfault_ota(sMemfaultOtaUserCtx *ctx) {
  if (!prv_wifi_connected_check("perform an OTA")) {
    return -1;
  }

  sMemfaultOtaUpdateHandler handler = {
    .user_ctx = ctx,
    .handle_update_available = prv_handle_ota_upload_available,
    .handle_download_complete = prv_handle_ota_download_complete,
  };

  MEMFAULT_LOG_INFO("Checking for OTA Update");

  int rv = memfault_esp_port_ota_update(&handler);
  if (rv == 0) {
    MEMFAULT_LOG_INFO("Up to date!");
  } else if (rv == 1) {
    MEMFAULT_LOG_INFO("Update available!");
  } else if (rv < 0) {
    MEMFAULT_LOG_ERROR("OTA update failed, rv=%d", rv);
  }

  return rv;
}

static int prv_memfault_ota_perform(int argc, char **argv) {
  sMemfaultOtaUserCtx user_ctx = {
    .perform_ota = true,
  };
  return prv_memfault_ota(&user_ctx);
}

static int prv_memfault_ota_check(int argc, char **argv) {
  sMemfaultOtaUserCtx user_ctx = {
    .perform_ota = false,
  };
  return prv_memfault_ota(&user_ctx);
}

static int prv_post_memfault_data(int argc, char **argv) {
  return memfault_esp_port_http_client_post_data();
}
#endif /* MEMFAULT_ESP_HTTP_CLIENT_ENABLE */

static int prv_chunk_data_export(int argc, char **argv) {
  memfault_data_export_dump_chunks();
  return 0;
}

void memfault_register_cli(void) {
  prv_timer_init();

  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "assert",
      .help = "Trigger an assert; optional arg of 1=assert(), 2=MEMFAULT_ASSERT(), 3=ESP_ERROR_CHECK(-1)",
      .hint = "assert type",
      .func = prv_esp32_assert_example,
  }));

  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "crash",
      .help = "Trigger a crash",
      .hint = "crash type",
      .func = memfault_demo_cli_cmd_crash,
  }));

  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "esp_crash",
      .help = "Trigger a timer isr crash",
      .hint = NULL,
      .func = prv_esp32_crash_example,
  }));

  ESP_ERROR_CHECK(esp_console_cmd_register(&(esp_console_cmd_t){
    .command = "leak",
    .help = "Allocate the specified number of bytes without freeing",
    .hint = NULL,
    .func = prv_esp32_leak_cmd,
  }));

  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "test_log",
      .help = "Writes test logs to log buffer",
      .hint = NULL,
      .func = memfault_demo_cli_cmd_test_log,
  }));

  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "trigger_logs",
      .help = "Trigger capture of current log buffer contents",
      .hint = NULL,
      .func = memfault_demo_cli_cmd_trigger_logs,
  }));

  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "clear_core",
      .help = "Clear an existing coredump",
      .hint = NULL,
      .func = memfault_demo_cli_cmd_clear_core,
  }));

  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "get_core",
      .help = "Get coredump info",
      .hint = NULL,
      .func = memfault_demo_cli_cmd_get_core,
  }));

  ESP_ERROR_CHECK(esp_console_cmd_register(&(esp_console_cmd_t) {
    .command = "coredump_size",
    .help = "Print the coredump storage capacity and the capacity required",
    .hint = NULL,
    .func = &memfault_demo_cli_cmd_coredump_size,
  }));

  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "get_device_info",
      .help = "Display device information",
      .hint = NULL,
      .func = memfault_demo_cli_cmd_get_device_info,
  }));

  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "export",
      .help = "Can be used to dump chunks to console or post via GDB",
      .hint = NULL,
      .func = prv_chunk_data_export,
  }));

  ESP_ERROR_CHECK(esp_console_cmd_register(&(esp_console_cmd_t){
    .command = "heartbeat",
    .help = "trigger an immediate capture of all heartbeat metrics",
    .hint = NULL,
    .func = prv_esp32_memfault_heartbeat,
  }));

  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "heartbeat_dump",
      .help = "Dump current Memfault metrics heartbeat state",
      .hint = NULL,
      .func = prv_esp32_memfault_heartbeat_dump,
  }));

#if MEMFAULT_ESP_HTTP_CLIENT_ENABLE
  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "post_chunks",
      .help = "Post Memfault data to cloud",
      .hint = NULL,
      .func = prv_post_memfault_data,
  }));

  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "memfault_ota_check",
      .help = "Checks Memfault to see if a new OTA is available",
      .hint = NULL,
      .func = prv_memfault_ota_check,
  }));

  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "memfault_ota_perform",
      .help = "Performs an OTA is an updates is available from Memfault",
      .hint = NULL,
      .func = prv_memfault_ota_perform,
  }));
#endif /* MEMFAULT_ESP_HTTP_CLIENT_ENABLE */
}
