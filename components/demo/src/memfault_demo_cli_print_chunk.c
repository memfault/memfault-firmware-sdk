//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! CLI command that dumps the coredump saved out over the console in such a way that the output
//! can be copy & pasted to a terminal and posted to the Memfault Cloud Service

#include "memfault/demo/cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memfault/core/data_packetizer.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/math.h"
#include "memfault/http/http_client.h"

#ifndef MEMFAULT_CLI_LOG_BUFFER_MAX_SIZE_BYTES
#  define MEMFAULT_CLI_LOG_BUFFER_MAX_SIZE_BYTES (80)
#endif

static void prv_write_curl_epilogue(void) {
  char url[MEMFAULT_HTTP_URL_BUFFER_SIZE];
  memfault_http_build_url(url, MEMFAULT_HTTP_API_CHUNKS_SUBPATH);
  MEMFAULT_LOG_RAW("| xxd -p -r | curl -X POST %s\\", url);
  MEMFAULT_LOG_RAW(" -H 'Memfault-Project-Key:%s'\\", g_mflt_http_client_config.api_key);
  MEMFAULT_LOG_RAW(" -H 'Content-Type:application/octet-stream' --data-binary @- -i");
  MEMFAULT_LOG_RAW("\nprint_chunk done");
}

int memfault_demo_cli_cmd_print_chunk(int argc, char *argv[]) {
  enum PrintFormat {
    kCurl,
    kHex,
  } fmt = kCurl;
  if (argc > 1) {
    if (0 == strcmp(argv[1], "curl")) {
      // fmt is kCurl by default
    } else if (0 == strcmp(argv[1], "hex")) {
      fmt = kHex;
    } else {
      MEMFAULT_LOG_ERROR("Usage: \"print_chunk\" or \"print_chunk <curl|hex>\"");
      return -1;
    }
  }

  struct PrintImpls {
    const char *prologue;
    const char *line_end;
    void (*write_epilogue)(void);
  } print_impl[] = {
      [kCurl] = {
          .prologue = "echo \\",
          .line_end = "\\",
          .write_epilogue = prv_write_curl_epilogue,
      },
      [kHex] = {
          .prologue = NULL,
          .line_end = "",
          .write_epilogue = NULL,
      },
  };

  const sPacketizerConfig cfg = {
    .enable_multi_packet_chunk = true,
  };
  sPacketizerMetadata metadata;
  if (!memfault_packetizer_begin(&cfg, &metadata)) {
    MEMFAULT_LOG_ERROR("No data to send!");
    return -1;
  }

  uint8_t buffer[MEMFAULT_CLI_LOG_BUFFER_MAX_SIZE_BYTES / 2];
  char hex_buffer[MEMFAULT_CLI_LOG_BUFFER_MAX_SIZE_BYTES];

  const size_t max_read_size = (MEMFAULT_CLI_LOG_BUFFER_MAX_SIZE_BYTES / 2) - strlen(print_impl[fmt].line_end) - 1;
  if (print_impl[fmt].prologue) {
    MEMFAULT_LOG_RAW("%s", print_impl[fmt].prologue);
  }

  eMemfaultPacketizerStatus packetizer_status;
  do {
    size_t read_size = max_read_size;
    packetizer_status = memfault_packetizer_get_next(buffer, &read_size);
    if (packetizer_status == kMemfaultPacketizerStatus_NoMoreData) {
      // we checked above if data is available so we should be able to read up to the end
      // of a chunk
      MEMFAULT_LOG_ERROR("Unable to read entire chunk");
      break;
    }

    for (uint32_t j = 0; j < read_size; ++j) {
      sprintf(&hex_buffer[j * 2], "%02x", buffer[j]);
    }
    strncpy(&hex_buffer[read_size * 2], print_impl[fmt].line_end,
            MEMFAULT_CLI_LOG_BUFFER_MAX_SIZE_BYTES - (read_size * 2));
    MEMFAULT_LOG_RAW("%s", hex_buffer);
  } while (packetizer_status != kMemfaultPacketizerStatus_EndOfChunk);

  if (print_impl[fmt].write_epilogue) {
    print_impl[fmt].write_epilogue();
  }
  return 0;
}
