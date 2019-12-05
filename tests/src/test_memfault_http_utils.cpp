#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

extern "C" {
  #include <stdbool.h>
  #include <stddef.h>
  #include <string.h>

  #include "memfault/core/platform/device_info.h"
  #include "memfault/http/http_client.h"
  #include "memfault/http/utils.h"

  void memfault_platform_get_device_info(struct MemfaultDeviceInfo *info) {
    *info = (struct MemfaultDeviceInfo) {
      .device_serial = "DEMOSERIAL",
      .software_type = "main",
      .software_version = "1.0.0",
      .hardware_version = "main-proto",
    };
  }

  sMfltHttpClientConfig g_mflt_http_client_config = {
    .api_key = "00112233445566778899aabbccddeeff",
    .api_host = "chunks.memfault.com",
    .api_no_tls = true,
    .api_port = 5002,
  };
}

TEST_GROUP(MfltHttpClientUtils){
  void setup() {
    mock().strictOrder();
  }
  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

typedef struct {
  char buf[256];
  size_t bytes_written;
} sHttpWriteCtx;

static bool prv_http_write_cb(const void *data, size_t data_len, void *ctx) {
  bool success = mock().actualCall(__func__).returnBoolValueOrDefault(true);
  if (!success) {
    return false;
  }

  sHttpWriteCtx *write_ctx = (sHttpWriteCtx *)ctx;
  memcpy(&write_ctx->buf[write_ctx->bytes_written], data, data_len);
  write_ctx->bytes_written += data_len;
  return true;
}

TEST(MfltHttpClientUtils, Test_MfltHttpClientPost) {
  mock().expectNCalls(7, "prv_http_write_cb");
  sHttpWriteCtx ctx = { 0 };
  bool success = memfault_http_start_chunk_post(prv_http_write_cb,
                                                            &ctx, 123);
  CHECK(success);
  const char *expected_string =
      "POST /api/v0/chunks/DEMOSERIAL HTTP/1.1\r\n"
      "Host:chunks.memfault.com\r\n"
      "User-Agent:MemfaultSDK/0.0.11\r\n"
      "Memfault-Project-Key:00112233445566778899aabbccddeeff\r\n"
      "Content-Type:application/octet-stream\r\n"
      "Content-Length:123\r\n\r\n";

  STRCMP_EQUAL(expected_string, ctx.buf);
}

TEST(MfltHttpClientUtils, Test_MfltHttpClientPostSendWriteFailure) {
  const size_t num_write_calls = 7;
  for (size_t i = 0; i < num_write_calls; i++) {
    if (i > 0) {
      mock().expectNCalls(i, "prv_http_write_cb");
    }
    mock().expectOneCall("prv_http_write_cb").andReturnValue(false);

    sHttpWriteCtx ctx = { 0 };
    bool success = memfault_http_start_chunk_post(prv_http_write_cb, &ctx, 10);
    CHECK(!success);
    mock().checkExpectations();
  }
}

static void prv_expect_parse_success(const char *rsp, size_t rsp_len, int expected_http_status) {
  // first we feed the message as an entire blob and confirm it parses
  sMemfaultHttpResponseContext ctx = { };
  bool done = memfault_http_parse_response(&ctx, rsp, rsp_len);
  CHECK(done);
  CHECK(!ctx.parse_error);
  LONGS_EQUAL(expected_http_status, ctx.http_status_code);
  LONGS_EQUAL(rsp_len, ctx.data_bytes_processed);

  // second we feed the message one at a time and confirm it parses
  memset(&ctx, 0x0, sizeof(ctx));
  for (size_t i = 0; i < rsp_len; i++) {
    done = memfault_http_parse_response(&ctx, &rsp[i], 1);
    LONGS_EQUAL(1, ctx.data_bytes_processed);
    LONGS_EQUAL(i == rsp_len - 1, done);
    CHECK(!ctx.parse_error);
    if (done) {
      LONGS_EQUAL(expected_http_status, ctx.http_status_code);
    }
  }
}

TEST(MfltHttpClientUtils, Test_MfltResponseParser202) {
  const char *rsp =
      "HTTP/1.1 202 Accepted\r\n"
      "Content-Type: text/plain; charset=utf-8\r\n"
      "Content-Length: 8\r\n"
      "Date: Wed, 27 Nov 2019 22:52:57 GMT\r\n"
      "Connection: keep-alive\r\n"
      "\r\n"
      "Accepted";

  const size_t rsp_len = strlen(rsp);
  sMemfaultHttpResponseContext ctx = { };
  bool done = memfault_http_parse_response(&ctx, rsp, rsp_len);
  CHECK(done);
  CHECK(!ctx.parse_error);
  LONGS_EQUAL(202, ctx.http_status_code);
  LONGS_EQUAL(rsp_len, ctx.data_bytes_processed);
  STRCMP_EQUAL("Accepted", ctx.http_body);
}

TEST(MfltHttpClientUtils, Test_MfltResponseParserNoContentLength) {
  const char *rsp =
      "HTTP/1.1 202 Accepted\r\n"
      "Content-Type: text/plain; charset=utf-8\r\n"
      "Content\rLength: 8\r\n"
      "Date: Wed, 27 Nov 2019 22:52:57 GMT\r\n"
      "Connection: keep-alive\r\n"
      "\r\n";

  sMemfaultHttpResponseContext ctx = { };
  bool done = memfault_http_parse_response(&ctx, rsp, strlen(rsp));
  CHECK(done);
  CHECK(!ctx.parse_error);
  LONGS_EQUAL(202, ctx.http_status_code);
  LONGS_EQUAL(0, ctx.content_length);
}


TEST(MfltHttpClientUtils, Test_MfltResponseParser411) {
  const char *good_response =
      "HTTP/1.1 411 Length Required\r\n"
      "Content-Type: application/json; charset=utf-8\r\n"
      "content-lenGTH: 67    \r\n"
      "Date: Wed, 27 Nov 2019 22:47:47 GMT\r\n"
      "Connection: keep-alive\r\n"
      "\r\n"
      "{\"error\":{\"message\":\"Content-Length must be set.\",\"http_code\":411}}";

  prv_expect_parse_success(good_response, strlen(good_response), 411);
}


TEST(MfltHttpClientUtils, Test_VeryLongBody) {
  const char *rsp_hdr =
      "HTTP/1.1 202 Accepted\r\n"
      "Content-Type: text/plain; charset=utf-8\r\n"
      "Content-Length: 1024\r\n"
      "Date: Wed, 27 Nov 2019 22:52:57 GMT\r\n"
      "Connection: keep-alive\r\n"
      "\r\n";
  const size_t rsp_hdr_len = strlen(rsp_hdr);
  char msg[rsp_hdr_len + 1024];
  memcpy(msg, rsp_hdr, rsp_hdr_len);
  prv_expect_parse_success(msg, sizeof(msg), 202);
}

static void prv_expect_parse_failure(const void *response, size_t response_len,
                                     eMfltHttpParseStatus expected_parse_error) {
  sMemfaultHttpResponseContext ctx = { };
  const bool done = memfault_http_parse_response(
      &ctx, response, response_len);
  CHECK(done);
  LONGS_EQUAL(expected_parse_error, ctx.parse_error)
}

TEST(MfltHttpClientUtils, Test_HttpResponseUnexpectedlyLong) {
  const static uint8_t response_parser[256] = "POST longdata";
  prv_expect_parse_failure(response_parser, sizeof(response_parser),
                           MfltHttpParseStatus_HeaderTooLongError);
}

TEST(MfltHttpClientUtils, Test_HeaderBeforeStatus) {
  const char *rsp =
      "Content-Type: text/plain; charset=utf-8\r\n"
      "HTTP/1.1 202 Accepted\r\n"
      "Content-Length: 1024\r\n"
      "Date: Wed, 27 Nov 2019 22:52:57 GMT\r\n"
      "Connection: keep-alive\r\n"
      "\r\n";
  prv_expect_parse_failure(rsp, strlen(rsp),
                           MfltHttpParseStatus_ParseStatusLineError);
}

TEST(MfltHttpClientUtils, Test_MfltResponseBadVersion) {
  const static char *rsp = "HTTZ/1.1 202 Accepted\r\n";
  prv_expect_parse_failure(rsp, strlen(rsp),
                           MfltHttpParseStatus_ParseStatusLineError);
}

TEST(MfltHttpClientUtils, Test_MfltResponseBadStatusCode) {
  const static char *rsp = "HTTP/1.1 2a2 Accepted\r\n";
  prv_expect_parse_failure(rsp, strlen(rsp),
                           MfltHttpParseStatus_ParseStatusLineError);
}

TEST(MfltHttpClientUtils, Test_MfltResponseShortStatusCode) {
  const static char *rsp = "HTTP/1.1 22 Accepted\r\n";
  prv_expect_parse_failure(rsp, strlen(rsp),
                           MfltHttpParseStatus_ParseStatusLineError);
}

TEST(MfltHttpClientUtils, Test_MfltResponseNoSpace) {
  const static char *rsp = "HTTP/1.1202 Accepted\r\n";
  prv_expect_parse_failure(rsp, strlen(rsp),
                           MfltHttpParseStatus_ParseStatusLineError);
}

TEST(MfltHttpClientUtils, Test_MfltResponseMalformedMinorVersion) {
  const static char *rsp = "HTTP/1.a 202 Accepted\r\n";
  prv_expect_parse_failure(rsp, strlen(rsp),
                           MfltHttpParseStatus_ParseStatusLineError);
}

TEST(MfltHttpClientUtils, Test_MfltResponseShort) {
  const static char *rsp_short = "HTTP/1.1 20\r\n";
  prv_expect_parse_failure(rsp_short, strlen(rsp_short),
                           MfltHttpParseStatus_ParseStatusLineError);

  const static char *rsp_short1 = "HTTP/1.1\r\n";
  prv_expect_parse_failure(rsp_short1, strlen(rsp_short1),
                           MfltHttpParseStatus_ParseStatusLineError);

  const static char *rsp_short2 = "HTTP/1.\r\n";
  prv_expect_parse_failure(rsp_short2, strlen(rsp_short2),
                           MfltHttpParseStatus_ParseStatusLineError);

  const static char *rsp_short3 = "HTTP/1\r\n";
  prv_expect_parse_failure(rsp_short3, strlen(rsp_short3),
                           MfltHttpParseStatus_ParseStatusLineError);
}

TEST(MfltHttpClientUtils, Test_MfltResponseBadContentLength) {
  const static char *rsp =
      "HTTP/1.1 202 Accepted\r\n"
      "Content-Length:1a\r\n\r\n";;
  prv_expect_parse_failure(rsp, strlen(rsp), MfltHttpParseStatus_ParseHeaderError);
}

TEST(MfltHttpClientUtils, Test_MfltResponseNoColonSeparator) {
  const static char *rsp =
      "HTTP/1.1 202 Accepted\r\n"
      "Content-Length&10\r\n\r\n";;
  prv_expect_parse_failure(rsp, strlen(rsp), MfltHttpParseStatus_ParseHeaderError);
}
