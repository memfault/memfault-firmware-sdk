#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! A collection of HTTP utilities *solely* for interacting with the Memfault REST API including
//!  - An API for generating HTTP Requests to Memfault REST API
//!  - An API for incrementally parsing HTTP Response Data [1]
//!
//! @note The expectation is that a typical application making use of HTTP will have a HTTP client
//! and parsing implementation already available to leverage.  This module is provided as a
//! reference and convenience implementation for very minimal environments.
//!
//! [1]: For more info on embedded-C HTTP parsing options, the following commit
//! in the Zephyr RTOS is a good starting point: https://mflt.io/35RWWwp

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

//! Writer invoked by calls to "memfault_http_start_chunk_post"
//!
//! For example, this would be where a user of the API would make a call to send() to push data
//! over a socket
typedef bool(*MfltHttpClientSendCb)(const void *data, size_t data_len, void *ctx);

//! Builds the HTTP 'Request-Line' and Headers for a POST to the Memfault Chunk Endpoint
//!
//! @note Upon completion of this call, a caller then needs to send the raw data received from
//! memfault_packetizer_get_next() out over the active connection.
//!
//! @param callback The callback invoked to send post request data.
//! @param ctx A user specific context that gets passed to 'callback' invocations.
//! @param content_body_length The length of the chunk payload to be sent. This value
//!  will be populated in the HTTP "Content-Length" header.
//!
//! @return true if the post was successful, false otherwise
bool memfault_http_start_chunk_post(
    MfltHttpClientSendCb callback, void *ctx, size_t content_body_length);

typedef enum MfltHttpParseStatus {
  kMfltHttpParseStatus_Ok = 0,
  MfltHttpParseStatus_ParseStatusLineError,
  MfltHttpParseStatus_ParseHeaderError,
  MfltHttpParseStatus_HeaderTooLongError,
} eMfltHttpParseStatus;

typedef enum MfltHttpParsePhase {
  kMfltHttpParsePhase_ExpectingStatusLine = 0,
  kMfltHttpParsePhase_ExpectingHeader,
  kMfltHttpParsePhase_ExpectingBody,
} eMfltHttpParsePhase;

typedef struct {
  //! true if a error occurred trying to parse the response
  eMfltHttpParseStatus parse_error;
  //! populated with the status code returned as part of the response
  int http_status_code;
  //! Pointer to http_body, may be truncated but always NULL terminated.
  //! This should only be used for debug purposes
  const char *http_body;
  //! The number of bytes processed by the last invocation of
  //! "memfault_http_parse_response"
  int data_bytes_processed;

  // For internal use only
  eMfltHttpParsePhase phase;
  int content_received;
  int content_length;
  size_t line_len;
  char line_buf[128];
} sMemfaultHttpResponseContext;


//! A *minimal* HTTP response parser for Memfault API calls
//!
//! @param ctx The context to be used while a parsing is in progress. It's
//!  expected that when a user first calls this function the context will be
//!  zero initialized
//! @param data The data to parse
//! @param data_len The length of the data to parse
//! @return True if parsing completed or false if more data is needed for the response
//!   Upon completion the 'parse_error' & 'http_status_code' fields can be checked
//!   within the 'ctx' for the results
bool memfault_http_parse_response(
    sMemfaultHttpResponseContext *ctx, const void *data, size_t data_len);

#ifdef __cplusplus
}
#endif
