//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! A minimal demo app for the NRF52 using the nRF5 v15.2 SDK that allows for crashes to be generated
//! from a RTT CLI and coredumps to be saved

#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "app_timer.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_drv_clock.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_pwr_mgmt.h"

#include "memfault/core/platform/core.h"
#include "mflt_cli.h"

static void timers_init(void) {
  ret_code_t err_code;

  err_code = app_timer_init();
  APP_ERROR_CHECK(err_code);
}

static void log_init(void) {
  // Note: We place the RTT buffers in a special section so they don't move around between
  // builds. This makes it easier for Segger to consistently lock onto them and display them
  extern uint32_t __rtt_start[];
  extern uint32_t __rtt_end[];
  memset(__rtt_start, 0x0, (uint32_t)__rtt_end - (uint32_t)__rtt_start);
  ret_code_t err_code = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(err_code);

  NRF_LOG_DEFAULT_BACKENDS_INIT();
}

typedef struct ElfNoteSection {
  uint32_t namesz;
  uint32_t descsz;
  uint32_t type;
  char  namedata[];
} sElfNoteSection;

// linker defined symbol
extern char g_gnu_build_id[];

static void prv_dump_build_id(void) {
  char build_id_sha[41] = { 0 };

  const sElfNoteSection *elf = (sElfNoteSection *)g_gnu_build_id;
  const char *data = &elf->namedata[elf->namesz]; // Skip over { 'G', 'N', 'U', '\0' }

  for (int i = 0; i < elf->descsz; i++) {
    snprintf(&build_id_sha[i*2], 3, "%02x", data[i]);
  }
  NRF_LOG_INFO("GNU Build Id: %s", build_id_sha);
}

int main(void) {
  nrf_drv_clock_init();
  nrf_drv_clock_lfclk_request(NULL);

  log_init();
  timers_init();
  mflt_cli_init();
  memfault_platform_boot();

  prv_dump_build_id();

  while (1) {
    mflt_cli_try_process();
    if (NRF_LOG_PROCESS() == false) {
      nrf_pwr_mgmt_run();
    }
  }
}
