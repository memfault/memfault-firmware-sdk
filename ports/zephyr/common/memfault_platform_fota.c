//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! Performs an OTA update using the Memfault Zephyr HTTP client

#include <errno.h>

#include "memfault/core/platform/core.h"

#if defined(CONFIG_MEMFAULT_ZEPHYR_FOTA_BACKEND_MCUBOOT)
  #include MEMFAULT_ZEPHYR_INCLUDE(dfu/flash_img.h)
  #include MEMFAULT_ZEPHYR_INCLUDE(dfu/mcuboot.h)
#endif

#include "memfault/components.h"
#include "memfault/ports/zephyr/fota.h"
#include "memfault/ports/zephyr/http.h"

#if defined(CONFIG_MEMFAULT_ZEPHYR_FOTA_BACKEND_MCUBOOT)

static bool s_fota_in_progress = false;

typedef struct {
  sMemfaultOtaInfo info;
  struct flash_img_context flash_ctx;
} sMemfaultOtaDownloadCtx;

  #if !defined(CONFIG_MEMFAULT_ZEPHYR_FOTA_DOWNLOAD_CALLBACK_CUSTOM)
bool memfault_zephyr_fota_download_callback(void) {
  int rv = boot_request_upgrade(BOOT_UPGRADE_TEST);
  if (rv != 0) {
    MEMFAULT_LOG_DEBUG("Error requesting upgrade, rv=%d", rv);
    return false;
  }

  // Reboot to apply the update
  MEMFAULT_LOG_INFO("OTA download complete! Rebooting...");
  MEMFAULT_REBOOT_MARK_RESET_IMMINENT(kMfltRebootReason_FirmwareUpdate);
  memfault_platform_reboot();

  // We should never reach this point, but if we do, return false to indicate an error
  return false;
}
  #endif

static bool prv_handle_update_available(const sMemfaultOtaInfo *info, void *user_ctx) {
  sMemfaultOtaDownloadCtx *ctx = (sMemfaultOtaDownloadCtx *)user_ctx;
  ctx->info = *info;

  MEMFAULT_LOG_INFO("Downloading OTA payload, size=%d bytes", ctx->info.size);
  return true;
}

static bool prv_handle_data(void *buf, size_t buf_len, void *user_ctx) {
  sMemfaultOtaDownloadCtx *ctx = (sMemfaultOtaDownloadCtx *)user_ctx;
  int rv = flash_img_buffered_write(&ctx->flash_ctx, buf, buf_len, false);
  if (rv != 0) {
    MEMFAULT_LOG_DEBUG("Error writing to flash, rv=%d", rv);
    return false;
  }

  int bytes_written = flash_img_bytes_written(&ctx->flash_ctx);
  int percent_complete = (bytes_written * 100) / ctx->info.size;  // todo int error possible?
  static int prev_percent_complete = 0;
  if ((percent_complete != prev_percent_complete)) {
    MEMFAULT_LOG_DEBUG("Downloaded %d/%d bytes (%d%%)", bytes_written, ctx->info.size,
                       (bytes_written * 100) / ctx->info.size);
  }
  prev_percent_complete = percent_complete;

  return true;
}

static bool prv_handle_download_complete(void *user_ctx) {
  // This function will not return if the download was successful when using the default
  // implementation. However, we allow a return value for custom implementations that may
  // want to return the result to the calling context.
  return memfault_zephyr_fota_download_callback();
}

int memfault_zephyr_fota_start(void) {
  // Ensure we don't start a new FOTA download if one is already in progress
  if (s_fota_in_progress) {
    MEMFAULT_LOG_INFO("FOTA already in progress");
    return -1;
  }

  s_fota_in_progress = true;
  uint8_t working_buf[CONFIG_IMG_BLOCK_BUF_SIZE];
  sMemfaultOtaDownloadCtx ctx;
  sMemfaultOtaUpdateHandler handler = {
    .buf = working_buf,
    .buf_len = sizeof(working_buf),
    .handle_update_available = prv_handle_update_available,
    .handle_data = prv_handle_data,
    .handle_download_complete = prv_handle_download_complete,
    .user_ctx = &ctx,
  };

  // Initialize a new flash image context when OTA is started. Note this OTA implementation does
  // not support resuming an OTA download, but if that support were added, this initialization
  // should only be done once. The flash context stores the number of bytes written to flash so far
  // and the current offset, among other state variables, which allows picking up where the last OTA
  // download session left off.
  //
  // Also note: slot1_partition is the default slot used by this flash image writer. For more
  // flexibility, we may want to parameterize the area used for OTA updates, which is supported by
  // this interface with flash_img_init_id(<ctx_ptr>, <area_id>).
  int rv = flash_img_init(&ctx.flash_ctx);
  if (rv != 0) {
    MEMFAULT_LOG_ERROR("Error preparing to write OTA to flash, rv=%d", rv);
    return rv;
  }

  rv = memfault_zephyr_port_ota_update(&handler);
  if (rv < 0) {
    MEMFAULT_LOG_ERROR("Error upgrading firmware, rv=%d", rv);
  } else if (rv == 0) {
    MEMFAULT_LOG_INFO("Up to date!");
  } else {
    // Added for completeness - we likely won't reach this branch since we generally reboot in
    // handler.handle_download_complete() after a successful download
    MEMFAULT_LOG_INFO("Update successful!");
  }
  s_fota_in_progress = false;
  return rv;
}

#endif /* CONFIG_MEMFAULT_ZEPHYR_FOTA_BACKEND_MCUBOOT */

#if defined(CONFIG_MEMFAULT_ZEPHYR_FOTA_BACKEND_DUMMY)
typedef struct {
  size_t total_size;
} sMemfaultOtaDownloadCtx;

static bool prv_handle_update_available(const sMemfaultOtaInfo *info, void *user_ctx) {
  sMemfaultOtaDownloadCtx *ctx = (sMemfaultOtaDownloadCtx *)user_ctx;
  ctx->total_size = info->size;
  MEMFAULT_LOG_INFO("Downloading OTA payload, size=%d bytes", (int)ctx->total_size);
  return true;
}

static bool prv_handle_data(void *buf, size_t buf_len, void *user_ctx) {
  // this is an example cli command so we just drop the data on the floor
  // a real implementation could save the data in this callback!
  return true;
}

static bool prv_handle_download_complete(void *user_ctx) {
  MEMFAULT_LOG_INFO("OTA download complete!");
  return true;
}

int memfault_zephyr_fota_start(void) {
  uint8_t working_buf[256];

  sMemfaultOtaDownloadCtx user_ctx;
  sMemfaultOtaUpdateHandler handler = {
    .buf = working_buf,
    .buf_len = sizeof(working_buf),
    .user_ctx = &user_ctx,
    .handle_update_available = prv_handle_update_available,
    .handle_data = prv_handle_data,
    .handle_download_complete = prv_handle_download_complete,
  };

  MEMFAULT_LOG_INFO("Checking for OTA update");
  int rv = memfault_zephyr_port_ota_update(&handler);
  if (rv == 0) {
    MEMFAULT_LOG_INFO("Up to date!");
  } else if (rv < 0) {
    MEMFAULT_LOG_INFO("OTA update failed, rv=%d, errno=%d", rv, errno);
  }
  return rv;
}

#endif /* CONFIG_MEMFAULT_ZEPHYR_FOTA_BACKEND_DUMMY */
