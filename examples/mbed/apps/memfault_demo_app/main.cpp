//! @file
//! @brief
//! main() runs in its own thread in the RTOS.  It runs a demo CLI
//! that provides commands for exercising the Mbed platform reference
//! implementation.  It also spins up a separate "blinky" thread that
//! is intended to make coredumps more interesting.

#include "memfault/core/compiler.h"
#include "memfault/core/debug_log.h"
#include "memfault/http/platform/http_client.h"

#include "blinky.h"
#include "cli.h"

// configure your project key in mbed_app.json
// find the key on memfault under settings > general
sMfltHttpClientConfig g_mflt_http_client_config = {
  .api_key = MBED_CONF_MEMFAULT_PROJECT_API_KEY,
};

int main(void) {
  // show a boot message to make it obvious when we reboot
  MEMFAULT_LOG_INFO("Memfault Mbed OS 5 demo app started...");

  // start an led blink thread to make coredumps more interesting
  blinky_init();

  // most of the demo is here
  cli_init();
  cli_forever();

  MEMFAULT_UNREACHABLE;
}
