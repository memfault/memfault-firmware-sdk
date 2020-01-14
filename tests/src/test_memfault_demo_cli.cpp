#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

extern "C" {
  #include <string.h>
  #include <stddef.h>

  #include "memfault/core/math.h"
  #include "memfault/core/platform/debug_log.h"
  #include "memfault/demo/cli.h"
  #include "memfault/http/http_client.h"
  #include "memfault/panics/platform/coredump.h"
  #include "mocks/mock_memfault_coredump.h"
}

sMfltHttpClientConfig g_mflt_http_client_config = {
    .api_key = "a1e284881e60450c957a5fbaae4e11de",
};

bool memfault_coredump_read(uint32_t offset, void *buf, size_t buf_len) {
  return (memfault_platform_coredump_storage_read(offset, buf, buf_len) != 0);
}

TEST_GROUP(MfltDemoCliTestGroup) {
  void setup() {
    // Set a size that will result in a more than a single line of output:
    g_mock_memfault_coredump_total_size = 140;
  }

   void teardown() {
     mock().checkExpectations();
     mock().clear();
   }
};

static void prv_setup_test_print_chunk_expectations(const char *const lines[], size_t num_lines) {
  mock().expectNCalls(2, "memfault_coredump_has_valid_coredump")
        .andReturnValue(true)
        .ignoreOtherParameters();
  mock().expectNCalls(4, "memfault_platform_coredump_storage_read").ignoreOtherParameters();
  for (unsigned int i = 0; i < num_lines; ++i) {
    mock().expectOneCall("memfault_platform_log_raw").withStringParameter("output", lines[i]);
  }
  // coredump should be cleared after it has been drained
  mock().expectOneCall("memfault_platform_coredump_storage_clear");
}

static void prv_setup_test_print_chunk_single_packet_expectations(const char *const lines[], size_t num_lines) {
  mock().expectNCalls(2, "memfault_coredump_has_valid_coredump")
        .andReturnValue(true)
        .ignoreOtherParameters();
  mock().expectNCalls(4, "memfault_platform_coredump_storage_read").ignoreOtherParameters();
  for (unsigned int i = 0; i < num_lines; ++i) {
    mock().expectOneCall("memfault_platform_log_raw").withStringParameter("output", lines[i]);
  }
  // coredump should be cleared after it has been drained
  mock().expectOneCall("memfault_platform_coredump_storage_clear");
}

static void prv_test_print_chunk_curl(int argc, char *argv[]) {
  const char *lines[] = {
      "echo \\",
      "0801ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff\\",
      "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff\\",
      "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff\\",
      "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffc2c\\",
      "| xxd -p -r | curl -X POST https://chunks.memfault.com/api/v0/chunks/DAABBCCDD\\",
      " -H 'Memfault-Project-Key:a1e284881e60450c957a5fbaae4e11de'\\",
      " -H 'Content-Type:application/octet-stream' --data-binary @- -i",
      "\nprint_chunk done",
  };
  prv_setup_test_print_chunk_expectations(lines, MEMFAULT_ARRAY_SIZE(lines));

  int retval = memfault_demo_cli_cmd_print_chunk(argc, (char **)argv);
  LONGS_EQUAL(0, retval);
}

TEST(MfltDemoCliTestGroup, Test_MfltDemoCliPrintCmdNoFormatUsesCurl) {
  char *argv[] = {"print_chunk"};
  prv_test_print_chunk_curl(MEMFAULT_ARRAY_SIZE(argv), (char **)argv);
}

TEST(MfltDemoCliTestGroup, Test_MfltDemoCliPrintCmdCurl) {
  char *argv[] = {"print_chunk", "curl"};
  prv_test_print_chunk_curl(MEMFAULT_ARRAY_SIZE(argv), (char **)argv);
}

TEST(MfltDemoCliTestGroup, Test_MfltDemoCliPrintCmdCurlSinglePacketChunk) {
  char *argv[] = {"print_chunk", "curl", "40"};

  const char *lines[] = {
      "echo \\",
      "488d0101ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff\\",
      "| xxd -p -r | curl -X POST https://chunks.memfault.com/api/v0/chunks/DAABBCCDD\\",
      " -H 'Memfault-Project-Key:a1e284881e60450c957a5fbaae4e11de'\\",
      " -H 'Content-Type:application/octet-stream' --data-binary @- -i",
      "\nprint_chunk done",
      "echo \\",
      "c023ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff\\",
      "| xxd -p -r | curl -X POST https://chunks.memfault.com/api/v0/chunks/DAABBCCDD\\",
      " -H 'Memfault-Project-Key:a1e284881e60450c957a5fbaae4e11de'\\",
      " -H 'Content-Type:application/octet-stream' --data-binary @- -i",
      "\nprint_chunk done",
      "echo \\",
      "c047ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff\\",
      "| xxd -p -r | curl -X POST https://chunks.memfault.com/api/v0/chunks/DAABBCCDD\\",
      " -H 'Memfault-Project-Key:a1e284881e60450c957a5fbaae4e11de'\\",
      " -H 'Content-Type:application/octet-stream' --data-binary @- -i",
      "\nprint_chunk done",
      "echo \\",
      "806bfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffc2c\\",
      "| xxd -p -r | curl -X POST https://chunks.memfault.com/api/v0/chunks/DAABBCCDD\\",
      " -H 'Memfault-Project-Key:a1e284881e60450c957a5fbaae4e11de'\\",
      " -H 'Content-Type:application/octet-stream' --data-binary @- -i",
      "\nprint_chunk done",
  };
  prv_setup_test_print_chunk_single_packet_expectations(lines, MEMFAULT_ARRAY_SIZE(lines));

  const size_t num_calls_needed = 4;
  for (size_t i = 0; i < num_calls_needed; i++) {
    int retval = memfault_demo_cli_cmd_print_chunk(MEMFAULT_ARRAY_SIZE(argv), (char **)argv);
    LONGS_EQUAL(0, retval);
  }
}

TEST(MfltDemoCliTestGroup, Test_MfltDemoCliPrintCmdHex) {
  const char *const lines[] = {
      "0801ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
      "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
      "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
      "fffffffffffffffffffffffffffffffffffffffffffffffffffc2c",
  };
  prv_setup_test_print_chunk_expectations(lines, MEMFAULT_ARRAY_SIZE(lines));

  char *argv[] = {"print_chunk", "hex"};
  int retval = memfault_demo_cli_cmd_print_chunk(MEMFAULT_ARRAY_SIZE(argv), (char **)argv);
  LONGS_EQUAL(0, retval);
}

TEST(MfltDemoCliTestGroup, Test_MfltDemoCliPrintCmdInvalidFormat) {
  mock().expectOneCall("memfault_platform_log").
         withIntParameter("level", kMemfaultPlatformLogLevel_Error).
         withStringParameter("output", "Usage: \"print_chunk\" or \"print_chunk <curl|hex> <chunk_size>\"");

  char *argv[] = {"print_chunk", "invalid"};
  int retval = memfault_demo_cli_cmd_print_chunk(MEMFAULT_ARRAY_SIZE(argv), (char **)argv);
  LONGS_EQUAL(-1, retval);
}


TEST(MfltDemoCliTestGroup, Test_MfltDemoCliPrintCmdNoData) {
  mock().expectOneCall("memfault_coredump_has_valid_coredump")
      .andReturnValue(false)
      .ignoreOtherParameters();
  mock().expectOneCall("memfault_platform_log").
         withIntParameter("level", kMemfaultPlatformLogLevel_Info).
         withStringParameter("output", "All data has been sent!");

  char *argv[] = {"print_chunk" };
  int retval = memfault_demo_cli_cmd_print_chunk(MEMFAULT_ARRAY_SIZE(argv), (char **)argv);
  LONGS_EQUAL(0, retval);
}

TEST(MfltDemoCliTestGroup, Test_MfltDemoCliInvalidChunkSize) {
  mock().expectOneCall("memfault_platform_log").
         withIntParameter("level", kMemfaultPlatformLogLevel_Error).
         withStringParameter("output", "Usage: chunk_size must be <= 40 bytes");

  char *argv[] = {"print_chunk", "curl", "41" };
  int retval = memfault_demo_cli_cmd_print_chunk(MEMFAULT_ARRAY_SIZE(argv), (char **)argv);
  LONGS_EQUAL(-1, retval);
}
