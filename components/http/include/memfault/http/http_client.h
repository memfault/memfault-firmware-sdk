#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! API when using the Memfault HTTP Client

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#define MEMFAULT_HTTP_URL_BUFFER_SIZE (128)

#ifndef MEMFAULT_HTTP_API_HOST
#  define MEMFAULT_HTTP_API_HOST "chunks.memfault.com"
#endif
#ifndef MEMFAULT_HTTP_API_PORT
#  define MEMFAULT_HTTP_API_PORT (443)
#endif

#define MEMFAULT_HTTP_API_PREFIX "/api/v0/"
#define MEMFAULT_HTTP_API_CHUNKS_SUBPATH "chunks"
#define MEMFAULT_HTTP_PROJECT_KEY_HEADER "Memfault-Project-Key"

#ifdef __cplusplus
extern "C" {
#endif

//! Configuration of the Memfault HTTP client.
typedef struct MfltHttpClientConfig {
  //! The project API key. This is a mandatory field.
  //! Go to app.memfault.com, then navigate to "Settings" to find your key.
  const char *api_key;
  //! The API host to use, NULL to use the default host.
  const char *api_host;
  //! Whether to not use TLS. When false, TLS/https will be used, otherwise plain text http will be used.
  bool api_no_tls;
  //! The TCP port to use or 0 to use the default port as defined by MEMFAULT_HTTP_API_PORT.
  uint16_t api_port;
} sMfltHttpClientConfig;

//! Global configuration of the Memfault HTTP client.
//! See @ref sMfltHttpClientConfig for information about each of the fields.
extern sMfltHttpClientConfig g_mflt_http_client_config;

//! Convenience macro to get the currently configured API hostname.
#define MEMFAULT_HTTP_GET_API_HOST() \
  (g_mflt_http_client_config.api_host ? g_mflt_http_client_config.api_host : MEMFAULT_HTTP_API_HOST)

//! Convenience macro to get the currently configured API port.
#define MEMFAULT_HTTP_GET_API_PORT() \
  (g_mflt_http_client_config.api_port ? g_mflt_http_client_config.api_port : MEMFAULT_HTTP_API_PORT)

//! Forward declaration of a HTTP client.
typedef struct MfltHttpClient sMfltHttpClient;

//! Writes a Memfault API URL for the specified subpath.
//! @param url_buffer Buffer where the URL should be written.
//! @param subpath Subpath to use.
//! @return true if the buffer was filled, false otherwise
bool memfault_http_build_url(char url_buffer[MEMFAULT_HTTP_URL_BUFFER_SIZE], const char *subpath);

//! Creates a new HTTP client. A client can be reused across requests.
//! This way, the cost of connection set up to the server will be shared with multiple requests.
//! @return The newly created client or NULL in case of an error.
//! @note Make sure to call memfault_platform_http_client_destroy() to close and clean up resources.
sMfltHttpClient *memfault_http_client_create(void);

typedef enum {
  kMfltPostDataStatus_Success = 0,
  kMfltPostDataStatus_NoDataFound = 1,
} eMfltPostDataStatus;

//! Posts Memfault data that is pending transmission to Memfault's services over HTTP.
//!
//! @return kMfltPostDataStatus_Success on success, kMfltPostDataStatus_NoDataFound
//! if no data was found or else an error code.
int memfault_http_client_post_data(sMfltHttpClient *client);

//! Waits until pending requests have been completed.
//! @param client The http client.
//! @return 0 on success, else error code
int memfault_http_client_wait_until_requests_completed(sMfltHttpClient *client,
                                                       uint32_t timeout_ms);

//! Destroys a HTTP client that was previously created using memfault_platform_http_client_create().
int memfault_http_client_destroy(sMfltHttpClient *client);

#ifdef __cplusplus
}
#endif
