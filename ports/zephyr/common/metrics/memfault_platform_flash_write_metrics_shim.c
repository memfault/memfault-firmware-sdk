//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! A devicetree-instantiated pass-through flash driver
//! ("memfault,flash-write-metrics-shim") that forwards read/write/erase
//! calls to an underlying flash device while counting bytes written into
//! the "flash_write_bytes" heartbeat metric. Insert it as the parent of a
//! "partitions" (fixed-partitions) node whose write traffic should be
//! tracked, e.g. a littlefs storage partition on external SPI NOR - no
//! changes to Zephyr's littlefs or flash_area code are required.

// clang-format off
#include "memfault/ports/zephyr/include_compatibility.h"

#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)
#include MEMFAULT_ZEPHYR_INCLUDE(device.h)
#include MEMFAULT_ZEPHYR_INCLUDE(drivers/flash.h)

#include "memfault/metrics/metrics.h"
// clang-format on

#define DT_DRV_COMPAT memfault_flash_write_metrics_shim

struct mflt_flash_shim_config {
  const struct device *flash_dev;
};

static int prv_shim_read(const struct device *dev, off_t offset, void *data, size_t len) {
  const struct mflt_flash_shim_config *cfg = dev->config;
  return flash_read(cfg->flash_dev, offset, data, len);
}

static int prv_shim_write(const struct device *dev, off_t offset, const void *data, size_t len) {
  const struct mflt_flash_shim_config *cfg = dev->config;
  int rc = flash_write(cfg->flash_dev, offset, data, len);
  if (rc == 0) {
    MEMFAULT_METRIC_ADD(flash_write_bytes, (uint32_t)len);
  }
  return rc;
}

static int prv_shim_erase(const struct device *dev, off_t offset, size_t size) {
  const struct mflt_flash_shim_config *cfg = dev->config;
  return flash_erase(cfg->flash_dev, offset, size);
}

static const struct flash_parameters *prv_shim_get_parameters(const struct device *dev) {
  const struct mflt_flash_shim_config *cfg = dev->config;
  return flash_get_parameters(cfg->flash_dev);
}

static int prv_shim_get_size(const struct device *dev, uint64_t *size) {
  const struct mflt_flash_shim_config *cfg = dev->config;
  return flash_get_size(cfg->flash_dev, size);
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void prv_shim_page_layout(const struct device *dev,
                                  const struct flash_pages_layout **layout, size_t *layout_size) {
  const struct mflt_flash_shim_config *cfg = dev->config;
  // Page geometry is a property of the underlying physical device; this shim
  // is a 1:1 passthrough so the underlying device's layout applies unchanged.
  const struct flash_driver_api *api = cfg->flash_dev->api;
  api->page_layout(cfg->flash_dev, layout, layout_size);
}
#endif

static int prv_shim_init(const struct device *dev) {
  const struct mflt_flash_shim_config *cfg = dev->config;
  if (!device_is_ready(cfg->flash_dev)) {
    return -ENODEV;
  }
  return 0;
}

static const struct flash_driver_api mflt_flash_shim_api = {
  .read = prv_shim_read,
  .write = prv_shim_write,
  .erase = prv_shim_erase,
  .get_parameters = prv_shim_get_parameters,
  .get_size = prv_shim_get_size,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
  .page_layout = prv_shim_page_layout,
#endif
};

#define MFLT_FLASH_SHIM_INIT(inst)                                                        \
  static const struct mflt_flash_shim_config mflt_flash_shim_config_##inst = {            \
    .flash_dev = DEVICE_DT_GET(DT_INST_PHANDLE(inst, flash)),                              \
  };                                                                                        \
  DEVICE_DT_INST_DEFINE(inst, prv_shim_init, NULL, NULL, &mflt_flash_shim_config_##inst,    \
                         POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY + 1, &mflt_flash_shim_api);

DT_INST_FOREACH_STATUS_OKAY(MFLT_FLASH_SHIM_INIT)
