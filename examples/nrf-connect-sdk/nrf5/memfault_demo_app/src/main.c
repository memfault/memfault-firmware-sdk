//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Example console main

#include <drivers/hwinfo.h>
#include <logging/log.h>
#include <memfault/components.h>
#include <memfault_ncs.h>
#include <stdio.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

//! wrapper to enclose logic around log_strdup
char *prv_conditional_log_strdup(char *str) {
#if !MEMFAULT_ZEPHYR_VERSION_GT(3, 1)
  #if defined(CONFIG_LOG) && !defined(CONFIG_LOG2)
  // Before zephyr 3.1, LOG was a different option from LOG2 and required
  // manually duplicating string argument values. Only required if CONFIG_LOG is
  // in use.
  return log_strdup(str);
  #endif
#endif

  return str;
}
static void prv_set_device_id(void) {
  uint8_t dev_id[16] = {0};
  char dev_id_str[sizeof(dev_id) * 2 + 1];
  char *dev_str = "UNKNOWN";

  // Obtain the device id
  ssize_t length = hwinfo_get_device_id(dev_id, sizeof(dev_id));

  // If this fails for some reason, use a fixed ID instead
  if (length <= 0) {
    dev_str = CONFIG_SOC_SERIES "-test";
    length = strlen(dev_str);
  } else {
    // Render the obtained serial number in hexadecimal representation
    for (size_t i = 0; i < length; i++) {
      (void)snprintf(&dev_id_str[i * 2], sizeof(dev_id_str), "%02x", dev_id[i]);
    }
    dev_str = dev_id_str;
  }

  LOG_INF("Device ID: %s", prv_conditional_log_strdup(dev_str));

  memfault_ncs_device_id_set(dev_str, length * 2);
}

void main(void) {
  LOG_INF("Booting Memfault sample app!");

  // Set the device id based on the hardware UID
  prv_set_device_id();

  memfault_device_info_dump();
}
