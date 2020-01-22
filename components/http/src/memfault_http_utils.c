//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! See header for more details

#include "memfault/http/utils.h"

#include <stdio.h>
#include <string.h>

#include "memfault/core/compiler.h"
#include "memfault/core/platform/device_info.h"
#include "memfault/http/http_client.h"

static bool prv_write_msg(MfltHttpClientSendCb write_callback, void *ctx,
                          const char *msg, size_t msg_len, size_t max_len) {
  if (msg_len >= max_len) {
    return false;
  }

  return write_callback(msg, msg_len, ctx);
}

bool memfault_http_start_chunk_post(
    MfltHttpClientSendCb write_callback, void *ctx, size_t content_body_length) {
  // Request built will look like this:
  //  POST /api/v0/chunks/<device_serial> HTTP/1.1\r\n
  //  Host:chunks.memfault.com\r\n
  //  User-Agent: MemfaultSDK/0.0.11\r\n
  //  Memfault-Project-Key:<PROJECT_KEY>\r\n
  //  Content-Type:application/octet-stream\r\n
  //  Content-Length:<content_body_length>\r\n
  //  \r\n

  sMemfaultDeviceInfo device_info;
  memfault_platform_get_device_info(&device_info);

  char buffer[100];
  const size_t max_msg_len = sizeof(buffer);
  size_t msg_len = (size_t)snprintf(buffer, sizeof(buffer),
                                    "POST /api/v0/chunks/%s HTTP/1.1\r\n",
                                    device_info.device_serial);
  if (!prv_write_msg(write_callback, ctx, buffer, msg_len, max_msg_len)) {
    return false;
  }

  // NB: All HTTP/1.1 requests must provide a Host Header
  //    https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Host
  msg_len = (size_t)snprintf(buffer, sizeof(buffer), "Host:%s\r\n",
                             MEMFAULT_HTTP_GET_API_HOST());
  if (!prv_write_msg(write_callback, ctx, buffer, msg_len, max_msg_len)) {
    return false;
  }

  #define USER_AGENT_HDR "User-Agent:MemfaultSDK/0.0.11\r\n"
  const size_t user_agent_hdr_len = MEMFAULT_STATIC_STRLEN(USER_AGENT_HDR);
  if (!write_callback(USER_AGENT_HDR, user_agent_hdr_len, ctx)) {
    return false;
  }

  msg_len = (size_t)snprintf(buffer, sizeof(buffer), "Memfault-Project-Key:%s\r\n",
                             g_mflt_http_client_config.api_key);

  #define CONTENT_TYPE "Content-Type:application/octet-stream\r\n"
  const size_t content_type_len = MEMFAULT_STATIC_STRLEN(CONTENT_TYPE);
  bool success = prv_write_msg(write_callback, ctx, buffer, msg_len, max_msg_len) &&
      write_callback(CONTENT_TYPE, content_type_len, ctx);
  if (!success) {
    return false;
  }

  msg_len = (size_t)snprintf(buffer, sizeof(buffer), "Content-Length:%d\r\n",
                             (int)content_body_length);

  #define END_HEADER_SECTION "\r\n"
  const size_t end_hdr_section_len = MEMFAULT_STATIC_STRLEN(END_HEADER_SECTION);
  success = prv_write_msg(write_callback, ctx, buffer, msg_len, max_msg_len) &&
      write_callback(END_HEADER_SECTION, end_hdr_section_len, ctx);
  return success;
}

static bool prv_is_number(char c) {
  return ((c) >= '0' && (c) <= '9');
}

static size_t prv_count_spaces(const char *line, size_t start_offset, size_t total_len) {
  size_t spaces_found = 0;
  for (; start_offset < total_len; start_offset++) {
    if (line[start_offset] != ' ') {
      break;
    }
    spaces_found++;
  }

  return spaces_found;
}

static char prv_lower(char in) {
  char maybe_lower_c = in | 0x20;
  if (maybe_lower_c >= 'a' && maybe_lower_c <= 'z') {
    // return lower case if value is actually in A-Z, a-z range
    return maybe_lower_c;
  }

  return in;
}

// depending on the libc used, strcasecmp isn't always available so let's
// use a simple variant here
static bool prv_strcasecmp(const char *line, const char *search_str, size_t str_len) {
  for (size_t idx = 0; idx < str_len; idx++) {
    char lower_c = prv_lower(line[idx]);

    if (lower_c != search_str[idx]) {
      return false;
    }
  }

  return true;
}

static int prv_str_to_dec(const char *buf, size_t buf_len, int *value_out) {
  int result = 0;
  size_t idx = 0;
  for (; idx < buf_len; idx++) {
    char c = buf[idx];
    if (c == ' ') {
      break;
    }

    if (!prv_is_number(c)) {
      // unexpected character encountered
      return -1;
    }

    int digit = c - '0';
    result = (result * 10) + digit;
  }

  *value_out = result;
  return idx;
}

//! @return true if parsing was successful, false if a parse error occurred
//! @note The only header we are interested in for the simple memfault response
//! parser is "Content-Length" to figure out how long the body is so that's all
//! this parser looks for
static bool prv_parse_header(
    char *line, size_t len, int *content_length_out) {
  #define CONTENT_LENGTH "content-length"
  const size_t content_hdr_len = MEMFAULT_STATIC_STRLEN(CONTENT_LENGTH);
  if (len < content_hdr_len) {
    return true;
  }

  size_t idx = 0;
  if (!prv_strcasecmp(line, CONTENT_LENGTH, content_hdr_len)) {
    return true;
  }

  idx += content_hdr_len;

  idx += prv_count_spaces(line, idx, len);
  if (line[idx] != ':') {
    return false;
  }
  idx++;

  idx += prv_count_spaces(line, idx, len);

  const int bytes_processed = prv_str_to_dec(&line[idx], len - idx, content_length_out);
  // should find at least one digit
  return (bytes_processed  > 0);
}

static bool prv_parse_status_line(char *line, size_t len, int *http_status) {
  #define HTTP_VERSION "HTTP/1."
  const size_t http_ver_len = MEMFAULT_STATIC_STRLEN(HTTP_VERSION);
  if (len < http_ver_len) {
    return false;
  }
  if (memcmp(line, HTTP_VERSION, http_ver_len) != 0) {
    return false;
  }
  size_t idx = http_ver_len;
  if (len < idx + 1) {
    return false;
  }

  // now we should have single byte minor version
  if (!prv_is_number(line[idx])) {
    return false;
  }
  idx++;

  // now we should have at least one space
  const size_t num_spaces = prv_count_spaces(line, idx, len);
  if (num_spaces == 0) {
    return false;
  }
  idx += num_spaces;

  const size_t status_code_num_digits = 3;
  size_t status_code_end = idx + status_code_num_digits;
  if (len < status_code_end ) {
    return false;
  }

  const int bytes_processed = prv_str_to_dec(&line[idx], status_code_end, http_status);

  // NB: the remainder line is the "Reason-Phrase" which we don't care about
  return (bytes_processed == status_code_num_digits);
}

static bool prv_is_cr_lf(char *buf) {
  return buf[0] == '\r' && buf[1] == '\n';
}

bool memfault_http_parse_response(
    sMemfaultHttpResponseContext *ctx, const void *data, size_t data_len) {
  ctx->data_bytes_processed = 0;

  const char *chars = (const char *)data;
  char *line_buf = &ctx->line_buf[0];
  for (size_t i = 0; i < data_len; i++, ctx->data_bytes_processed++) {
    const char c = chars[i];
    if (ctx->phase == kMfltHttpParsePhase_ExpectingBody) {
      // Just eat the message body so we can handle response lengths of arbitrary size
      ctx->content_received++;

      if (ctx->line_len < (sizeof(ctx->line_buf) - 1)) {
        line_buf[ctx->line_len] = c;
        ctx->line_len++;
      }

      bool done = (ctx->content_received == ctx->content_length);
      if (!done) {
        continue;
      }
      ctx->line_buf[ctx->line_len] = '\0';
      ctx->http_body = ctx->line_buf;
      ctx->data_bytes_processed++;
      return done;
    }

    if (ctx->line_len >= sizeof(ctx->line_buf)) {
      ctx->parse_error = MfltHttpParseStatus_HeaderTooLongError;
      return true;
    }

    line_buf[ctx->line_len] = c;
    ctx->line_len++;

    if (ctx->line_len < 2) {
      continue;
    }

    const size_t len = ctx->line_len - 2;
    if (prv_is_cr_lf(&line_buf[len])) {
      ctx->line_len  = 0;
      line_buf[len] = '\0';

      // The first line in a http response is the HTTP "Status-Line"
      if (ctx->phase == kMfltHttpParsePhase_ExpectingStatusLine) {
        if (!prv_parse_status_line(line_buf, len, &ctx->http_status_code)) {
          ctx->parse_error = MfltHttpParseStatus_ParseStatusLineError;
          return true;
        }
        ctx->phase = kMfltHttpParsePhase_ExpectingHeader;
      } else if (ctx->phase == kMfltHttpParsePhase_ExpectingHeader) {
        if (!prv_parse_header(line_buf, len, &ctx->content_length)) {
          ctx->parse_error = MfltHttpParseStatus_ParseHeaderError;
          return true;
        }

        if (len != 0) {
          continue;
        }
        // We've reached the end of headers marker
        if (ctx->content_length == 0) {
          // no body to read
          return true;
        }
        ctx->phase = kMfltHttpParsePhase_ExpectingBody;
      }
    }
  }
  return false;
}
