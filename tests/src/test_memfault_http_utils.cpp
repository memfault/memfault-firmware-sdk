#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

extern "C" {
  #include <limits.h>
  #include <stdbool.h>
  #include <stddef.h>
  #include <string.h>

  #include "memfault/core/math.h"
  #include "memfault/core/platform/device_info.h"
  #include "memfault/http/http_client.h"
  #include "memfault/http/utils.h"
  #include "memfault/version.h"

  const struct MemfaultDeviceInfo g_device_info_default = {
      .device_serial = "DEMOSERIAL",
      .software_type = "main",
      .software_version = "1.0.0",
      .hardware_version = "main-proto",
  };
  static struct MemfaultDeviceInfo g_device_info;

  void memfault_platform_get_device_info(struct MemfaultDeviceInfo *info) {
    *info = g_device_info;
  }

  sMfltHttpClientConfig g_mflt_http_client_config;
}

TEST_GROUP(MfltHttpClientUtils){
  void setup() {
    g_mflt_http_client_config = (sMfltHttpClientConfig) {
      .api_key = "00112233445566778899aabbccddeeff",
    };
    g_device_info = g_device_info_default;

    mock().strictOrder();
  }
  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

TEST(MfltHttpClientUtils, Test_MfltHttpClientOverrides) {
  STRCMP_EQUAL("chunks.memfault.com", MEMFAULT_HTTP_GET_CHUNKS_API_HOST());
  STRCMP_EQUAL("device.memfault.com", MEMFAULT_HTTP_GET_DEVICE_API_HOST());

  LONGS_EQUAL(443, MEMFAULT_HTTP_GET_CHUNKS_API_PORT());
  LONGS_EQUAL(443, MEMFAULT_HTTP_GET_DEVICE_API_PORT());

  g_mflt_http_client_config = (sMfltHttpClientConfig) {
    .api_key = "00112233445566778899aabbccddeeff",
    .disable_tls = false,
    .chunks_api = {
      .host = "override-chunks.memfault.com",
      .port = 1,
    },
    .device_api = {
      .host = "override-api.memfault.com",
      .port = 2,
    },
  };

  STRCMP_EQUAL("override-chunks.memfault.com", MEMFAULT_HTTP_GET_CHUNKS_API_HOST());
  STRCMP_EQUAL("override-api.memfault.com", MEMFAULT_HTTP_GET_DEVICE_API_HOST());

  LONGS_EQUAL(1, MEMFAULT_HTTP_GET_CHUNKS_API_PORT());
  LONGS_EQUAL(2, MEMFAULT_HTTP_GET_DEVICE_API_PORT());
}

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
  mock().expectNCalls(11, "prv_http_write_cb");
  sHttpWriteCtx ctx = { 0 };
  bool success = memfault_http_start_chunk_post(prv_http_write_cb,
                                                            &ctx, 123);
  CHECK(success);
  const char *expected_string =
      "POST /api/v0/chunks/DEMOSERIAL HTTP/1.1\r\n"
      "Host:chunks.memfault.com\r\n"
      "User-Agent:MemfaultSDK/" MEMFAULT_SDK_VERSION_STR "\r\n"
      "Memfault-Project-Key:00112233445566778899aabbccddeeff\r\n"
      "Content-Type:application/octet-stream\r\n"
      "Content-Length:123\r\n\r\n";

  STRCMP_EQUAL(expected_string, ctx.buf);
}

TEST(MfltHttpClientUtils, Test_MfltHttpClientPostSendWriteFailure) {
  const size_t num_write_calls = 11;
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

TEST(MfltHttpClientUtils, Test_MfltHttpClientGetOtaPayloadUrl) {
  mock().expectNCalls(26, "prv_http_write_cb");
  sHttpWriteCtx ctx = { 0 };
  bool success = memfault_http_get_latest_ota_payload_url(prv_http_write_cb, &ctx);
  CHECK(success);

  const char *expected_string =
      "GET /api/v0/releases/latest/url?&device_serial=DEMOSERIAL&hardware_version=main-proto&software_type=main&current_version=1.0.0 HTTP/1.1\r\n"
      "Host:device.memfault.com\r\n"
      "User-Agent:MemfaultSDK/" MEMFAULT_SDK_VERSION_STR "\r\n"
      "Memfault-Project-Key:00112233445566778899aabbccddeeff\r\n"
      "\r\n";

  STRCMP_EQUAL(expected_string, ctx.buf);
}

TEST(MfltHttpClientUtils, Test_MfltHttpClientGetOtaPayloadUrlWriteFailure) {
  const size_t num_write_calls = 26;
  for (size_t i = 0; i < num_write_calls; i++) {
    if (i > 0) {
      mock().expectNCalls(i, "prv_http_write_cb");
    }
    mock().expectOneCall("prv_http_write_cb").andReturnValue(false);

    sHttpWriteCtx ctx = { 0 };
    bool success = memfault_http_get_latest_ota_payload_url(prv_http_write_cb, &ctx);
    CHECK(!success);
    mock().checkExpectations();
  }
}

TEST(MfltHttpClientUtils, Test_MfltHttpClientGetOtaPayloadUrlEncodeFailure) {
  mock().expectNCalls(1, "prv_http_write_cb");
  sHttpWriteCtx ctx = { 0 };
  g_device_info.device_serial = "++++++++++++++++";
  bool success = memfault_http_get_latest_ota_payload_url(prv_http_write_cb, &ctx);
  CHECK(!success);
  mock().checkExpectations();
}

TEST(MfltHttpClientUtils, Test_MfltHttpClientGetOtaPayload) {
  mock().expectNCalls(8, "prv_http_write_cb");
  sHttpWriteCtx ctx = { 0 };

  const char *url = "https://example.ota.payload.com/path/to/ota/payload/yay";
  bool success = memfault_http_get_ota_payload(prv_http_write_cb, &ctx,
                                               url, strlen(url));
  CHECK(success);

  const char *expected_string =
      "GET /path/to/ota/payload/yay HTTP/1.1\r\n"
      "Host:example.ota.payload.com\r\n"
      "User-Agent:MemfaultSDK/" MEMFAULT_SDK_VERSION_STR "\r\n"
      "\r\n";

  STRCMP_EQUAL(expected_string, ctx.buf);
}

TEST(MfltHttpClientUtils, Test_MfltHttpClientGetOtaPayloadNoPath) {
  mock().expectNCalls(8, "prv_http_write_cb");
  sHttpWriteCtx ctx = { 0 };

  const char *url = "https://example.ota.payload.com";
  bool success = memfault_http_get_ota_payload(prv_http_write_cb, &ctx,
                                               url, strlen(url));
  CHECK(success);

  const char *expected_string =
      "GET / HTTP/1.1\r\n"
      "Host:example.ota.payload.com\r\n"
      "User-Agent:MemfaultSDK/" MEMFAULT_SDK_VERSION_STR "\r\n"
      "\r\n";

  STRCMP_EQUAL(expected_string, ctx.buf);
}

TEST(MfltHttpClientUtils, Test_MfltHttpClientGetOtaPayloadBadUrl) {
  sHttpWriteCtx ctx = { 0 };

  const char *url = "ftp://";
  bool success = memfault_http_get_ota_payload(prv_http_write_cb, &ctx,
                                               url, strlen(url));
  CHECK(!success);
}

TEST(MfltHttpClientUtils, Test_MfltHttpClientGetOtaPayloadFailure) {
  const size_t num_write_calls = 8;
  for (size_t i = 0; i < num_write_calls; i++) {
    if (i > 0) {
      mock().expectNCalls(i, "prv_http_write_cb");
    }
    mock().expectOneCall("prv_http_write_cb").andReturnValue(false);

    sHttpWriteCtx ctx = { 0 };

    const char *url = "https://example.ota.payload.com/path/to/ota/payload/yay";
    bool success = memfault_http_get_ota_payload(prv_http_write_cb, &ctx,
                                                 url, strlen(url));
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

  // third we only parse the header.
  memset(&ctx, 0x0, sizeof(ctx));
  done = memfault_http_parse_response_header(&ctx, rsp, rsp_len);
  CHECK(done);
  CHECK(!ctx.parse_error);
  LONGS_EQUAL(expected_http_status, ctx.http_status_code);
  // the message-body should not be included in the data_bytes_processed
  // so we add the parsed content_length to recover the total size
  const size_t total_rsp_len = (size_t)(ctx.data_bytes_processed + ctx.content_length);
  LONGS_EQUAL(rsp_len, total_rsp_len);
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
  bool done = memfault_http_parse_response(
      &ctx, response, response_len);
  CHECK(done);
  LONGS_EQUAL(expected_parse_error, ctx.parse_error);

  // should also fail if we only parse the header
  memset(&ctx, 0x0, sizeof(ctx));
  done = memfault_http_parse_response_header(
      &ctx, response, response_len);
  CHECK(done);
  LONGS_EQUAL(expected_parse_error, ctx.parse_error);
}

TEST(MfltHttpClientUtils, Test_HttpResponseUnexpectedlyLong) {
  // One really long header we don't care about but must tolerate.
  const char *rsp =
      "HTTP/1.1 202 Accepted\r\n"
      "Content-Type: text/plain; charset=utf-8\r\n"
      "ReallyLongHeader:ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789;ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789\r\n"
      "Content-Length: 8\r\n"
      "Date: Wed, 27 Nov 2019 22:52:57 GMT\r\n"
      "Connection: keep-alive\r\n"
      "\r\n"
      "Accepted"; // Body, 8 bytes

    prv_expect_parse_success(rsp, strlen(rsp), 202);
}

TEST(MfltHttpClientUtils, Test_HttpResponseUnexpectedlyLongFirstLine) {
  // One really long header as the first line should fail.
  const char *rsp =
      "ReallyLongHeader:ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789;ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789\r\n"
      "HTTP/1.1 202 Accepted\r\n"
      "Content-Type: text/plain; charset=utf-8\r\n"
      "Content-Length: 8\r\n"
      "Date: Wed, 27 Nov 2019 22:52:57 GMT\r\n"
      "Connection: keep-alive\r\n"
      "\r\n"
      "Accepted"; // Body, 8 bytes

    prv_expect_parse_failure(rsp, strlen(rsp),
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

TEST(MfltHttpClientUtils, Test_MfltResponseOverLongStatusCode) {
  LONGS_EQUAL(2147483647, INT_MAX);  // sanity check
  const static char *rsp = "HTTP/1.1 2147483648\r\n";
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
  const static char *rsp_non_base_10_digit = "HTTP/1.1 202 Accepted\r\n"
                                             "Content-Length:1a\r\n\r\n";
  prv_expect_parse_failure(rsp_non_base_10_digit, strlen(rsp_non_base_10_digit),
                           MfltHttpParseStatus_ParseHeaderError);

  const static char *rsp_value_too_large = "HTTP/1.1 202 Accepted\r\n"
                                           "Content-Length:2147483648\r\n\r\n";
  prv_expect_parse_failure(rsp_value_too_large, strlen(rsp_value_too_large),
                           MfltHttpParseStatus_ParseHeaderError);
}

TEST(MfltHttpClientUtils, Test_MfltResponseNoColonSeparator) {
  const static char *rsp =
      "HTTP/1.1 202 Accepted\r\n"
      "Content-Length&10\r\n\r\n";
  prv_expect_parse_failure(rsp, strlen(rsp), MfltHttpParseStatus_ParseHeaderError);
}

static void prv_check_result(const char *uri, const char *host,
                             const char *path, bool expect_success,
                             int expected_port) {
  sMemfaultUriInfo uri_info;
  bool success = memfault_http_parse_uri(uri, strlen(uri), &uri_info);
  CHECK(success == expect_success);
  if (!success) {
    return;
  }

  LONGS_EQUAL(uri_info.host_len, strlen(host));
  MEMCMP_EQUAL(uri_info.host, host, uri_info.host_len);

  LONGS_EQUAL(uri_info.path_len, strlen(path));
  if (uri_info.path_len == 0) {
    CHECK(uri_info.path == NULL);
  } else {
    MEMCMP_EQUAL(uri_info.path, path, uri_info.path_len);
  }

  const char *http_scheme = "http://";
  const bool is_http_scheme = (strncmp(uri, http_scheme, strlen(http_scheme)) == 0);

  if (expected_port == -1) {
    expected_port = is_http_scheme ? 80 : 443;
  }

  LONGS_EQUAL(is_http_scheme ? kMemfaultUriScheme_Http : kMemfaultUriScheme_Https,
              uri_info.scheme);
  LONGS_EQUAL(expected_port, uri_info.port);
}

static void prv_uri_parse_check(const char *scheme, const char *host,
                                const char *path, bool expect_success) {
  char uri[256];
  int rv = snprintf(uri, sizeof(uri), "%s%s%s", scheme, host, path);
  CHECK(rv > 0 && (size_t)rv < sizeof(uri));

  prv_check_result(uri, host, path, expect_success, -1);
}

static void prv_uri_with_port_parse_check(const char *scheme, const char *host, int port,
                                          const char *path, bool expect_success) {
  char uri[256];
  int rv = snprintf(uri, sizeof(uri), "%s%s:%d%s", scheme, host, port, path);
  CHECK(rv > 0 && (size_t)rv < sizeof(uri));
  prv_check_result(uri, host, path, expect_success, port);
}

TEST(MfltHttpClientUtils, Test_MfltParseHttpAndHttpsUris) {
  const char *valid_prefixes[] = { "https://", "http://", "http://username:password@" };
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(valid_prefixes); i++) {
    const bool expect_success = true;
    const char *p = valid_prefixes[i];

    const char *paths[] = {
      "/",
      "/a/b/c/d?param1=1&param2=2",
      ""
    };

    for (size_t j = 0; j < MEMFAULT_ARRAY_SIZE(paths); j++) {
      const char *path = paths[j];

      //! NB: We'll test with the three valid types of hostnames
      //!   IP-literal: "[::1]"
      //!   IPv4address: "127.0.0.1"
      //!   registered-name: "www.mywebsite.com"

      prv_uri_parse_check(p, "www.mysite.com", path, expect_success);
      prv_uri_with_port_parse_check(p, "www.mysite.com", 80, path, expect_success);

      prv_uri_parse_check(p, "127.0.0.1", path, expect_success);
      prv_uri_with_port_parse_check(p, "127.0.0.1", 80, path, expect_success);

      prv_uri_parse_check(p, "[::1]", "/", expect_success);
      prv_uri_with_port_parse_check(p, "[::1]", 443, path, expect_success);

      prv_uri_parse_check(p, "[2001:0db8:85a3:0000:0000:8a2e:0370:7334]", path,
                          expect_success);
      prv_uri_with_port_parse_check(p, "[2001:0db8:85a3:0000:0000:8a2e:0370:7334]", 832,
                                    path, expect_success);
    }
  }
}

TEST(MfltHttpClientUtils, Test_MfltParseUriWithUnsupportedScheme) {
  const char *unsupported_schemes[] = { "https:/", "http//", "http:/", "ftp://" };
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(unsupported_schemes); i++) {
    const bool expect_success = false;
    const char *scheme = unsupported_schemes[i];
    prv_uri_parse_check(scheme, "www.mysite.com", "/api/v0/a/b/c", expect_success);
  }
}

TEST(MfltHttpClientUtils, Test_MfltParseUriWithMalformedHost) {
  const char *malformed_hosts[] = {
    "[::1",
    "username:password@",
  };

  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(malformed_hosts); i++) {
    const bool expect_success = false;
    const char *h = malformed_hosts[i];
    prv_uri_parse_check("https://", h, "/api/v0/a/b/c", expect_success);
    prv_uri_with_port_parse_check("http://", h, 8000, "/api/v0/a/b/c", expect_success);
  }
}

TEST(MfltHttpClientUtils, Test_MfltParseUriWithBogusPort) {
  const char *hosts_with_bogus_ports[] = {
    "[::1]:8abc",
    "www.example.com:80z",
  };

  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(hosts_with_bogus_ports); i++) {
    const bool expect_success = false;
    const char *h = hosts_with_bogus_ports[i];
    prv_uri_parse_check("https://", h, "/api/v0/a/b/c", expect_success);
  }
}

TEST(MfltHttpClientUtils, Test_MfltParseUriWithOnlyScheme) {
  const bool expect_success = false;
  prv_check_result("http", NULL, NULL, expect_success, 0);
  prv_check_result("http:/", NULL, NULL, expect_success, 0);
  prv_check_result("http://", NULL, NULL, expect_success, 0);
  prv_check_result("http:///", NULL, NULL, expect_success, 0);
}

TEST(MfltHttpClientUtils, Test_UrlEncode) {
  const struct inputs {
    const char *instring;
    const char *outstring;
  } input_vectors[] = {
    {"a", "a"},
    {"a b", "a%20b"},
    {"a+b%", "a%2Bb%25"},
    // reserved characters
    {"!#$&'()*+,/:;=?@[]", "%21%23%24%26%27%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5D"},
    // unreserved characters
    {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~",
     "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~"},
  };

  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(input_vectors); i++) {
    const char *instring = input_vectors[i].instring;
    const char *outstring = input_vectors[i].outstring;
    char result[strlen(outstring) + 1];
    memset(result, '\0', sizeof(result));

    int rv = memfault_http_urlencode(instring, strlen(instring), result, sizeof(result));

    LONGS_EQUAL(0, rv);
    STRCMP_EQUAL(outstring, result);
  }

  // test with a buffer that is too small
  {
    char result[2];
    memset(result, 0xA5, sizeof(result));
    int rv = memfault_http_urlencode("!", 1, result, sizeof(result));
    LONGS_EQUAL(-1, rv);
  }
  // perfectly sized buffer
  {
    char result[5];
    memset(result, 0xA5, sizeof(result));
    int rv = memfault_http_urlencode("a!", 2, result, sizeof(result));
    LONGS_EQUAL(0, rv);
    STRCMP_EQUAL("a%21", result);
  }
  // buffer 1 char too small
  {
    char result[4];
    memset(result, 0xA5, sizeof(result));
    int rv = memfault_http_urlencode("a!", 2, result, sizeof(result));
    LONGS_EQUAL(-1, rv);
  }
  {
    char result[4];
    memset(result, 0xA5, sizeof(result));
    int rv = memfault_http_urlencode("!a", 2, result, sizeof(result));
    LONGS_EQUAL(-1, rv);
  }

  // few more edge cases
  {
    int rv = memfault_http_urlencode("1", 1, NULL, 0);
    LONGS_EQUAL(-1, rv);
    rv = memfault_http_urlencode(NULL, 0, NULL, 1);
    LONGS_EQUAL(-1, rv);
  }
}

TEST(MfltHttpClientUtils, Test_UrlNeedsEscape) {
  const struct inputs {
    const char *instring;
    bool needs_escape;
  } input_vectors[] = {
    {"", false},
    {"a", false},
    {"a b", true},
    {"a+b%", true},
    // reserved characters
    {"!#$&'()*+,/:;=?@[]", true},
    // unreserved characters
    {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~", false},
  };
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(input_vectors); i++) {
    const char *instring = input_vectors[i].instring;
    const bool needs_escape = input_vectors[i].needs_escape;
    const bool result = memfault_http_needs_escape(instring, strlen(instring));

    CHECK_TEXT(needs_escape == result, instring);
  }
}
