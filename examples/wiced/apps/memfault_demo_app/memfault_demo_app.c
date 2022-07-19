//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//!  Memfault WICED Demo App

#include <inttypes.h>

#include "command_console.h"
#include "command_console_wifi.h"
#include "platform_dct.h"
#include "wwd_assert.h"
#include "wiced.h"
#include "wiced_apps_common.h"
#include "wiced_dct_common.h"

#include "memfault/core/debug_log.h"
#include "memfault/core/platform/core.h"
#include "memfault/demo/cli.h"
#include "memfault/http/http_client.h"
#include "memfault_platform_wiced.h"

#define MAX_LINE_LENGTH  (128)
#define MAX_HISTORY_LENGTH (10)

static char line_buffer[MAX_LINE_LENGTH];
static char history_buffer_storage[MAX_LINE_LENGTH * MAX_HISTORY_LENGTH];

// Find your key on https://app.memfault.com/ under 'Settings':
sMfltHttpClientConfig g_mflt_http_client_config = {
    .api_key = "<YOUR PROJECT KEY HERE>",
};

static int prv_get_core_region(int argc, char *argv[]) {
  uint32_t flash_start;
  uint32_t flash_end;
  if (!memfault_platform_get_spi_start_and_end_addr(&flash_start, &flash_end)) {
    MEMFAULT_LOG_ERROR("Failed to get SPI start & end addresses");
    return 0;
  }
  MEMFAULT_LOG_INFO("Coredump region SPI start: 0x%"PRIx32" end: 0x%"PRIx32" (size: %"PRIu32" bytes)",
                    flash_start, flash_end, flash_end - flash_start);
  return 0;
}

static const command_t commands[] = {
    {"get_core", memfault_demo_cli_cmd_get_core, 0, NULL, NULL, NULL, "Get coredump info"},
    {"clear_core", memfault_demo_cli_cmd_clear_core, 0, NULL, NULL, NULL, "Clear an existing coredump"},
    {"post_core", memfault_demo_cli_cmd_post_core, 0, NULL, NULL, NULL, "Post coredump to Memfault"},
    {"crash", memfault_demo_cli_cmd_crash, 1, NULL, NULL, "<type>"ESCAPE_SPACE_PROMPT, "Trigger a crash"},

    {"get_core_region", prv_get_core_region, 0, NULL, NULL, NULL, "Get coredump SPI region info"},
    {"get_device_info", memfault_demo_cli_cmd_get_device_info, 0, NULL, NULL, NULL, "Get device info"},

    // Copied from command_console_wifi.h -- not using WIFI_COMMANDS to avoid dragging all the wifi code
    {"join", join, 2, NULL, NULL,
     "<ssid> <open|wpa_aes|wpa_tkip|wpa2|wpa2_tkip|wpa2_fbt> [key] [channel] [ip netmask gateway]"ESCAPE_SPACE_PROMPT,
    "Join an AP. DHCP assumed if no IP address provided"},

    CMD_TABLE_END,
};

void application_start(void) {
  wiced_init();
  memfault_platform_boot();
  WPRINT_APP_INFO( ( "Memfault Test App on WICED at your service!\n" ) );

  // Expose the test app functionality through console commands:
  command_console_init(STDIO_UART, MAX_LINE_LENGTH, line_buffer, MAX_HISTORY_LENGTH, history_buffer_storage, " ");
  console_add_cmd_table(commands);

  WPRINT_APP_INFO(("Type 'help' to list available commands...\n"));
}
