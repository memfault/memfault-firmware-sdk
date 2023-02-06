/* Console example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "argtable3/argtable3.h"
#include "cmd_decl.h"
#include "driver/uart.h"
#include "esp_console.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_task.h"
#include "esp_vfs_dev.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "led.h"
#include "linenoise/linenoise.h"
#include "memfault/components.h"
#include "memfault/esp_port/cli.h"
#include "memfault/esp_port/core.h"
#include "memfault/esp_port/http_client.h"
#include "memfault/esp_port/version.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "example";

/* Console command history can be stored to and loaded from a file.
 * The easiest way to do this is to use FATFS filesystem on top of
 * wear_levelling library.
 */
#if CONFIG_STORE_HISTORY

  #define MOUNT_PATH "/data"
  #define HISTORY_PATH MOUNT_PATH "/history.txt"

static void initialize_filesystem() {
  static wl_handle_t wl_handle;
  const esp_vfs_fat_mount_config_t mount_config = {.max_files = 4, .format_if_mount_failed = true};
  esp_err_t err = esp_vfs_fat_spiflash_mount(MOUNT_PATH, "storage", &mount_config, &wl_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
    return;
  }
}
#endif  // CONFIG_STORE_HISTORY

static void initialize_nvs() {
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);
}

// Name change at version 4.x
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
  #define CONFIG_CONSOLE_UART_NUM CONFIG_ESP_CONSOLE_UART_NUM
#endif

static void initialize_console() {
  /* Disable buffering on stdin and stdout */
  setvbuf(stdin, NULL, _IONBF, 0);
  setvbuf(stdout, NULL, _IONBF, 0);

  /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
  esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
  /* Move the caret to the beginning of the next line on '\n' */
  esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

  /* Install UART driver for interrupt-driven reads and writes */
  ESP_ERROR_CHECK(uart_driver_install(CONFIG_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0));

  /* Tell VFS to use UART driver */
  esp_vfs_dev_uart_use_driver(CONFIG_CONSOLE_UART_NUM);

  /* Initialize the console */
  esp_console_config_t console_config = {
    .max_cmdline_args = 8,
    .max_cmdline_length = 256,
#if CONFIG_LOG_COLORS
    .hint_color = atoi(LOG_COLOR_CYAN)
#endif
  };
  ESP_ERROR_CHECK(esp_console_init(&console_config));

  /* Configure linenoise line completion library */
  /* Enable multiline editing. If not set, long commands will scroll within
   * single line.
   */
  linenoiseSetMultiLine(1);

  /* Tell linenoise where to get command completions and hints */
  linenoiseSetCompletionCallback(&esp_console_get_completion);
  linenoiseSetHintsCallback((linenoiseHintsCallback *)&esp_console_get_hint);

  /* Set command history size */
  linenoiseHistorySetMaxLen(100);

#if CONFIG_STORE_HISTORY
  /* Load command history from filesystem */
  linenoiseHistoryLoad(HISTORY_PATH);
#endif
}

// Put this buffer in the IRAM region. Accesses on the instruction bus must be word-aligned
// while data accesses don't have to be. See "1.3.1 Address Mapping" in the ESP32 technical
// reference manual.
MEMFAULT_ALIGNED(4) static IRAM_ATTR uint8_t s_my_buf[10];
void *g_unaligned_buffer;

#if CONFIG_MEMFAULT_APP_OTA

static bool prv_handle_ota_upload_available(void *user_ctx) {
  // set blue when performing update
  led_set_color(kLedColor_Blue);

  MEMFAULT_LOG_INFO("Starting OTA download ...");
  return true;
}

static bool prv_handle_ota_download_complete(void *user_ctx) {
  MEMFAULT_LOG_INFO("OTA Update Complete, Rebooting System");

  // The pc & lr which result in the reboot can always be *optionally* recorded
  void *pc;
  MEMFAULT_GET_PC(pc);
  void *lr;
  MEMFAULT_GET_LR(lr);
  sMfltRebootTrackingRegInfo reg_info = {
    .pc = (uint32_t)pc,
    .lr = (uint32_t)lr,
  };
  // Note: "reg_info" may be NULL if no register information collection is desired
  memfault_reboot_tracking_mark_reset_imminent(kMfltRebootReason_FirmwareUpdate, &reg_info);

  esp_restart();
  return true;
}

static void prv_memfault_ota(void) {
  if (!memfault_esp_port_wifi_connected()) {
    return;
  }

  sMemfaultOtaUpdateHandler handler = {
    .user_ctx = NULL,
    .handle_update_available = prv_handle_ota_upload_available,
    .handle_download_complete = prv_handle_ota_download_complete,
  };

  MEMFAULT_LOG_INFO("Checking for OTA Update");

  int rv = memfault_esp_port_ota_update(&handler);
  if (rv == 0) {
    MEMFAULT_LOG_INFO("Up to date!");
    led_set_color(kLedColor_Green);
  } else if (rv == 1) {
    MEMFAULT_LOG_INFO("Update available!");
  } else if (rv < 0) {
    MEMFAULT_LOG_ERROR("OTA update failed, rv=%d", rv);
    led_set_color(kLedColor_Red);
  }
}
#else
static void prv_memfault_ota(void) {}
#endif  // CONFIG_MEMFAULT_APP_OTA

#if CONFIG_MEMFAULT_APP_WIFI_AUTOJOIN
void memfault_esp_port_wifi_autojoin(void) {
  if (memfault_esp_port_wifi_connected()) {
    return;
  }

  char *ssid, *pass;
  wifi_load_creds(&ssid, &pass);
  if ((ssid == NULL) || (pass == NULL) || (strnlen(ssid, 64) == 0) || (strnlen(pass, 64) == 0)) {
    MEMFAULT_LOG_DEBUG("No WiFi credentials found");
    return;
  }
  MEMFAULT_LOG_DEBUG("Starting WiFi Autojoin ...");
  bool result = wifi_join(ssid, pass);
  if (!result) {
    MEMFAULT_LOG_DEBUG("Failed to join WiFi network");
  }
}

#endif  // CONFIG_MEMFAULT_APP_WIFI_AUTOJOIN

// Periodically post any Memfault data that has not yet been posted.
static void prv_poster_task(void *args) {
  const uint32_t interval_sec = 60;
  const TickType_t delay_ms = (1000 * interval_sec) / portTICK_PERIOD_MS;

  MEMFAULT_LOG_INFO("Data poster task up and running every %" PRIu32 "s.", interval_sec);

  while (true) {
    // count the number of times this task has run
    memfault_metrics_heartbeat_add(MEMFAULT_METRICS_KEY(PosterTaskNumSchedules), 1);
    // attempt to autojoin wifi, if configured
    memfault_esp_port_wifi_autojoin();

    // if connected, post any memfault data
    if (memfault_esp_port_wifi_connected()) {
      MEMFAULT_LOG_DEBUG("Checking for memfault data to send");
      int err = memfault_esp_port_http_client_post_data();
      // if the check-in succeeded, set green, otherwise clear.
      // gives a quick eyeball check that the app is alive and well
      led_set_color((err == 0) ? kLedColor_Green : kLedColor_Red);
    }

    // check for OTA update
    prv_memfault_ota();

    // sleep
    vTaskDelay(delay_ms);
  }
}

// Example showing how to use the task watchdog
#if MEMFAULT_TASK_WATCHDOG_ENABLE

SemaphoreHandle_t g_example_task_lock;

static void prv_example_task(void *args) {
  (void)args;

  // set up the semaphore used to programmatically make this task stuck
  #if MEMFAULT_FREERTOS_PORT_USE_STATIC_ALLOCATION != 0
  static StaticSemaphore_t s_memfault_lock_context;
  g_example_task_lock = xSemaphoreCreateRecursiveMutexStatic(&s_memfault_lock_context);
  #else
  g_example_task_lock = xSemaphoreCreateRecursiveMutex();
  #endif

  MEMFAULT_ASSERT(g_example_task_lock != NULL);

  // this task runs every 250ms and gets/puts a semaphore. if the semaphore is
  // claimed, the task watchdog will eventually trip and mark this task as stuck
  const uint32_t interval_ms = 250;

  MEMFAULT_LOG_INFO("Task watchdog example task running every %" PRIu32 "ms.", interval_ms);
  while (true) {
    // enable the task watchdog
    MEMFAULT_TASK_WATCHDOG_START(example_task);

    // get the semaphore. if we can't get it, the task watchdog should
    // eventually trip
    xSemaphoreTakeRecursive(g_example_task_lock, portMAX_DELAY);
    xSemaphoreGiveRecursive(g_example_task_lock);

    // disable the task watchdog now that this task is done in this run
    MEMFAULT_TASK_WATCHDOG_STOP(example_task);

    vTaskDelay(interval_ms / portTICK_PERIOD_MS);
  }
}

static void prv_task_watchdog_timer_callback(MEMFAULT_UNUSED TimerHandle_t handle) {
  memfault_task_watchdog_check_all();
}

static void prv_initialize_task_watchdog(void) {
  memfault_task_watchdog_init();

  // create a timer that runs the watchdog check once a second
  const char *const pcTimerName = "TaskWatchdogTimer";
  const TickType_t xTimerPeriodInTicks = pdMS_TO_TICKS(1000);

  TimerHandle_t timer;

  #if MEMFAULT_FREERTOS_PORT_USE_STATIC_ALLOCATION != 0
  static StaticTimer_t s_task_watchdog_timer_context;
  timer = xTimerCreateStatic(pcTimerName, xTimerPeriodInTicks, pdTRUE, NULL,
                             prv_task_watchdog_timer_callback, &s_task_watchdog_timer_context);
  #else
  timer =
    xTimerCreate(pcTimerName, xTimerPeriodInTicks, pdTRUE, NULL, prv_task_watchdog_timer_callback);
  #endif

  MEMFAULT_ASSERT(timer != 0);

  xTimerStart(timer, 0);

  // create and start the example task
  const portBASE_TYPE res = xTaskCreate(prv_example_task, "example_task", ESP_TASK_MAIN_STACK, NULL,
                                        ESP_TASK_MAIN_PRIO, NULL);
  MEMFAULT_ASSERT(res == pdTRUE);
}
#else
static void prv_initialize_task_watchdog(void) {
  // task watchdog disabled, do nothing
}
#endif

// This task started by cpu_start.c::start_cpu0_default().
void app_main() {
#if !CONFIG_MEMFAULT_AUTOMATIC_INIT
  memfault_boot();
#endif
  extern void memfault_platform_device_info_boot(void);
  memfault_platform_device_info_boot();
  memfault_device_info_dump();

  g_unaligned_buffer = &s_my_buf[1];

  initialize_nvs();

#if CONFIG_STORE_HISTORY
  initialize_filesystem();
#endif

  initialize_console();

  led_init();

  prv_initialize_task_watchdog();

  // We need another task to post data since we block waiting for user
  // input in this task.
  const portBASE_TYPE res =
    xTaskCreate(prv_poster_task, "poster", ESP_TASK_MAIN_STACK, NULL, ESP_TASK_MAIN_PRIO, NULL);
  MEMFAULT_ASSERT(res == pdTRUE);

  /* Register commands */
  esp_console_register_help_command();
  register_system();
  register_wifi();
  register_app();

#if MEMFAULT_COMPACT_LOG_ENABLE
  MEMFAULT_COMPACT_LOG_SAVE(kMemfaultPlatformLogLevel_Info, "This is a compact log example");
#endif

  /* Prompt to be printed before each line.
   * This can be customized, made dynamic, etc.
   */
  const char *prompt = LOG_COLOR_I "esp32> " LOG_RESET_COLOR;

  const char banner[] = "\n\n" MEMFAULT_BANNER_COLORIZED;

  puts(banner);

  /* Figure out if the terminal supports escape sequences */
  int probe_status = linenoiseProbe();
  if (probe_status) { /* zero indicates success */
    printf("\n"
           "Your terminal application does not support escape sequences.\n"
           "Line editing and history features are disabled.\n"
           "On Windows, try using Putty instead.\n");
    linenoiseSetDumbMode(1);
#if CONFIG_LOG_COLORS
    /* Since the terminal doesn't support escape sequences,
     * don't use color codes in the prompt.
     */
    prompt = "esp32> ";
#endif  // CONFIG_LOG_COLORS
  }

  /* Main loop */
  while (true) {
    /* Get a line using linenoise (blocking call).
     * The line is returned when ENTER is pressed.
     */
    char *line = linenoise(prompt);
    if (line == NULL) { /* Ignore empty lines */
      continue;
    }
    /* Add the command to the history */
    linenoiseHistoryAdd(line);
#if CONFIG_STORE_HISTORY
    /* Save command history to filesystem */
    linenoiseHistorySave(HISTORY_PATH);
#endif

    /* Try to run the command */
    int ret;
    esp_err_t err = esp_console_run(line, &ret);
    if (err == ESP_ERR_NOT_FOUND) {
      printf("Unrecognized command\n");
    } else if (err == ESP_ERR_INVALID_ARG) {
      // command was empty
    } else if (err == ESP_OK && ret != ESP_OK) {
      printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(err));
    } else if (err != ESP_OK) {
      printf("Internal error: %s\n", esp_err_to_name(err));
    }
    /* linenoise allocates line buffer on the heap, so need to free it */
    linenoiseFree(line);
  }
}
