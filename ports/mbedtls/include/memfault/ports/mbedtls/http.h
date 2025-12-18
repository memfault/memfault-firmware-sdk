#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! MbedTLS specific http utility for interfacing with Memfault HTTP utilities

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

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
int memfault_mbedtls_port_ota_update(const sMemfaultOtaUpdateHandler *handler);

//! Query Memfault's Release Mgmt Infra for an OTA update
//!
//! @param download_url populated with a string containing the download URL to use
//! if an OTA update is available.
//!
//! @note After use, memfault_mbedtls_port_release_download_url() must be called
//!  to free the memory where the download URL is stored.
//!
//! @return
//!   < 0 Error while trying to figure out if an update was available
//!     0 Check completed successfully - No new update available
//!     1 New update is available and download_url has been populated with
//!       the url to use for download
int memfault_mbedtls_port_get_download_url(char **download_url);

//! Releases the memory returned from memfault_mbedtls_port_get_download_url()
int memfault_mbedtls_port_release_download_url(char **download_url);

#ifdef __cplusplus
}
#endif
