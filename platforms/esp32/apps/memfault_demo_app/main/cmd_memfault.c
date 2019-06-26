#include "driver/periph_ctrl.h"
#include "driver/timer.h"
#include "esp_console.h"
#include "esp_err.h"

#include "memfault/core/debug_log.h"
#include "memfault/core/math.h"
#include "memfault/core/platform/debug_log.h"
#include "memfault/demo/cli.h"
#include "memfault/http/platform/http_client.h"
#include "memfault/panics/assert.h"
#include "memfault/panics/platform/coredump.h"

#define TIMER_DIVIDER         16  //  Hardware timer clock divider
#define TIMER_SCALE_TICKS_PER_MS    ((TIMER_BASE_CLK / TIMER_DIVIDER) / 1000)  // convert counter value to milliseconds

static void IRAM_ATTR prv_recursive_crash(int depth) {
  if (depth == 15) {
    ESP_ERROR_CHECK(-1);
    // TODO:
    // MEMFAULT_ASSERT_RECORD(depth);
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
  uint8_t buf2[200];
  prv_check1(buf2);
}

void prv_check3(const void *buf) {
  uint8_t buf3[300];
  prv_check2(buf3);
}

void prv_check4(void) {
  uint8_t buf4[400];
  prv_check3(buf4);
}

static void IRAM_ATTR prv_timer_group0_isr(void *para) {
  // Always clear the interrupt:
  TIMERG0.int_clr_timers.t0 = 1;

  // Crash from ISR:
//  prv_recursive_crash(0);
  ESP_ERROR_CHECK(-1);
}

static void prv_timer_init(void)
{
  const timer_config_t config = {
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
  timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0x00000000ULL);
  timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, timer_interval_ms * TIMER_SCALE_TICKS_PER_MS);
  timer_set_alarm(TIMER_GROUP_0, TIMER_0, TIMER_ALARM_EN);
  timer_start(TIMER_GROUP_0, TIMER_0);
}

// jump through some hoops to trick the compiler into doing an unaligned 64 bit access
extern void *g_unaligned_buffer;

static int prv_crash_example(int argc, char** argv) {
  int crash_type =  0;

  if (argc >= 2) {
    crash_type = atoi(argv[1]);
  }

  if (crash_type == 0) {
    ESP_ERROR_CHECK(10);
  } else if (crash_type == 1) {
    // The linker is smart; when using a literal here that does not exist in .text it fails!
    void (*bad_func_call)(void) = (void *)g_unaligned_buffer;
    bad_func_call();
  } else if (crash_type == 2) {
    // Crash in timer ISR:
    prv_timer_start(10);
  } else if (crash_type == 3) {
    prv_recursive_crash(0);
  } else if (crash_type == 4) {
    prv_check4();
  } else if (crash_type == 5) {
    // g_unaligned_buffer is stored in IRAM and therefore accesses need to be word-aligned
    uint64_t *buf = g_unaligned_buffer;
    *buf = 0xbadcafe0000;
  }
  return 0;
}

void register_memfault(void) {
  prv_timer_init();

  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "crash",
      .help = "trigger a crash",
      .hint = "crash type",
      .func = &prv_crash_example,
  }));

  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "delete_core",
      .help = "delete the core",
      .hint = NULL,
      .func = &memfault_demo_cli_cmd_delete_core,
  }));

  ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t) {
      .command = "post_core",
      .help = "post the core",
      .hint = NULL,
      .func = &memfault_demo_cli_cmd_post_core,
  }));
}
