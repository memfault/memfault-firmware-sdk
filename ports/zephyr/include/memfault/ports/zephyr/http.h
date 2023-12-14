#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Zephyr specific http utility for interfacing with Memfault HTTP utilities

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

//! Installs the root CAs Memfault uses (the certs in "memfault/http/root_certs.h")
//!
//! @note: MUST be called prior to LTE network init (calling NRF's lte_lc_init_and_connect())
//!  for the settings to take effect
//!
//! @return 0 on success, else error code
int memfault_zephyr_port_install_root_certs(void);

//! Sends diagnostic data to the Memfault cloud chunks endpoint
//!
//! @note If the socket is unavailable after a timeout of 5 seconds, or if the underlying socket
//! send operation returns with error, the function will abort the transfer. Otherwise, this
//! function will block and return once posting of chunk data has completed.
//!
//! For more info see https://mflt.io/data-to-cloud
//!
//! @return 0 on success, else error code
int memfault_zephyr_port_post_data(void);

typedef struct MemfaultOtaInfo {
  // The size, in bytes, of the OTA payload.
  size_t size;
} sMemfaultOtaInfo;

typedef struct {
  //! Caller provided buffer to be used for the duration of the OTA lifecycle
  void *buf;
  size_t buf_len;

  //! Optional: Context for use by the caller
  void *user_ctx;

  //! Called if a new ota update is available
  //! @return true to continue, false to abort the OTA download
  bool (*handle_update_available)(const sMemfaultOtaInfo *info, void *user_ctx);

  //! Invoked as bytes are downloaded for the OTA update
  //!
  //! @return true to continue, false to abort the OTA download
  bool (*handle_data)(void *buf, size_t buf_len, void *user_ctx);

  //! Called once the entire ota payload has been downloaded
  bool (*handle_download_complete)(void *user_ctx);
} sMemfaultOtaUpdateHandler;

//! Handler which can be used to run OTA update using Memfault's Release Mgmt Infra
//! For more details see:
//!  https://mflt.io/release-mgmt
//!
//! @param handler Context with info necessary to perform an OTA. See struct
//!  definition for more details.
//!
//! @note This function is blocking. 'handler' callbacks will be invoked prior to the function
//!   returning.
//!
//! @return
//!   < 0 Error while trying to figure out if an update was available
//!     0 Check completed successfully - No new update available
//!     1 New update is available and handlers were invoked
int memfault_zephyr_port_ota_update(const sMemfaultOtaUpdateHandler *handler);

//! Query Memfault's Release Mgmt Infra for an OTA update
//!
//! @param download_url populated with a string containing the download URL to use
//! if an OTA update is available.
//!
//! @note After use, memfault_zephyr_port_release_download_url() must be called
//!  to free the memory where the download URL is stored.
//!
//! @return
//!   < 0 Error while trying to figure out if an update was available
//!     0 Check completed successfully - No new update available
//!     1 New update is available and download_url has been populated with
//!       the url to use for download
int memfault_zephyr_port_get_download_url(char **download_url);

//! Releases the memory returned from memfault_zephyr_port_get_download_url()
int memfault_zephyr_port_release_download_url(char **download_url);

//!
//! Utility functions for manually posting memfault data.

//! Context structure used to carry state information about the HTTP connection
typedef struct {
  int sock_fd;
  struct zsock_addrinfo *res;
} sMemfaultHttpContext;

//! Open a socket to the Memfault chunks upload server
//!
//! This function is the simplest way to connect to Memfault. Internally this function combines the
//! required socket operations into a single function call. See the other socket functions for
//! advanced usage.
//!
//! @param ctx If the socket is opened successfully, this will be populated with
//! the connection state for the other HTTP functions below
//!
//! @note After use, memfault_zephyr_port_http_close_socket() must be called
//!  to close the socket and free any associated memory.
//! @note In the case of an error, it is not required to call memfault_zephyr_port_http_close_socket
//!
//! @return
//!   0 : Success
//! < 0 : Error
int memfault_zephyr_port_http_open_socket(sMemfaultHttpContext *ctx);

//! Creates a socket to the Memfault chunks upload server
//!
//! @param ctx If the socket is created successfully, this will be populated with
//! the connection state for the other HTTP functions below
//!
//! @note This function only creates the socket. The user should also configure the socket with
//! desired options before calling memfault_zephyr_port_http_configure_socket and
//! memfault_zephyr_port_http_connect_socket.
//!
//! @note After use, memfault_zephyr_port_http_close_socket() must be called
//!  to close the socket and free any associated memory
//! @note In the case of an error, it is not required to call memfault_zephyr_port_http_close_socket
//!
//! @return
//!   0 : Success
//! < 0 : Error
int memfault_zephyr_port_http_create_socket(sMemfaultHttpContext *ctx);

//! Configures a socket to the Memfault chunks upload server
//!
//! @param ctx Current HTTP context containing the socket to configure.
//!
//! @note This function only configures the socket TLS options for use with Memfault. The user
//! should also configure the socket with desired options before calling
//! memfault_zephyr_port_http_connect_socket.
//!
//! @note After use, memfault_zephyr_port_http_close_socket() must be called
//!  to close the socket and free any associated memory
//! @note In the case of an error, it is not required to call memfault_zephyr_port_http_close_socket
//!
//! @return
//!   0 : Success
//! < 0 : Error
int memfault_zephyr_port_http_configure_socket(sMemfaultHttpContext *ctx);

//! Connect the socket to the chunks upload server
//!
//! @param ctx Current HTTP context containing the socket to connect with.
//!
//! @note After use, memfault_zephyr_port_http_close_socket() must be called
//!  to close the socket and free any associated memory
//! @note In the case of an error, it is not required to call memfault_zephyr_port_http_close_socket
//!
//! @return
//!   0 : Success
//! < 0 : Error
int memfault_zephyr_port_http_connect_socket(sMemfaultHttpContext *ctx);

//! Close a socket previously opened with
//! memfault_zephyr_port_http_open_socket()
void memfault_zephyr_port_http_close_socket(sMemfaultHttpContext *ctx);

//! Test if the socket is open
bool memfault_zephyr_port_http_is_connected(sMemfaultHttpContext *ctx);

//! Identical to memfault_zephyr_port_post_data() but uses the already-opened
//! socket to send data
//!
//! @param ctx Connection context previously opened with
//! memfault_zephyr_port_http_open_socket()
//! @return 0 on success, 1 on error
int memfault_zephyr_port_http_upload_sdk_data(sMemfaultHttpContext *ctx);

//! Similar to memfault_zephyr_port_http_upload_sdk_data(), but instead of using
//! the SDK packetizer functions under the hood, send the data passed into this
//! function.
//!
//! Typically this function is used to send data from pre-packetized data; for
//! example, data that may have been stored outside of the Memfault SDK
//! internally-managed buffers, or data coming from an external source (another
//! chip running the Memfault SDK).
//!
//! @param ctx Connection context previously opened with
//! memfault_zephyr_port_http_open_socket()
//!
//! @return
//!   0 : Success
//! < 0 : Error
int memfault_zephyr_port_http_post_chunk(sMemfaultHttpContext *ctx, void *p_data, size_t data_len);

#ifdef __cplusplus
}
#endif
