#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

//! Returns whether or not there is Memfault data to send
//!
//! This function is called by memfault_http_client_post_data() when data is being pushed to the
//! memfault service.
//!
//! There is a weak implementation defined by default which checks for locally collected memfault
//! data. If multiple MCUs are forwarding data to the ESP32 for transport, this function can
//! be overridden to check the sources for data as well.
//!
//! @return true if there is memfault data available to send, false otherwise
bool memfault_esp_port_data_available(void);

//! Fills buffer with a Memfault "chunk" when there is data available
//!
//! This function is called by memfault_http_client_post_data() when data is being pushed to the
//! memfault service.
//!
//! There is a weak implementation defined by default which checks for locally collected memfault
//! data. If multiple MCUs are forwarding data to the ESP32 for transport, this function can
//! be overridden to check the sources for data as well.
//!
//! @param[out] buf The buffer to copy data to be sent into
//! @param[in,out] buf_len The size of the buffer to copy data into. On return, populated
//! with the amount of data, in bytes, that was copied into the buffer.
//!
//! @return true if the buffer was filled, false otherwise
bool memfault_esp_port_get_chunk(void *buf, size_t *buf_len);

//! Record vprintf log lines into the Memfault log buffer.
//!
//! Normally installed with esp_log_set_vprintf() as part of the Memfault
//! integration memfault_boot() function, this function is exposed if the
//! esp_log_set_vprintf() call is disabled with
//! CONFIG_MEMFAULT_LOG_USE_VPRINTF_HOOK=n, and the user still wants to record
//! logs into the Memfault logging system.
//!
//! @param[in] fmt Format string
//! @param[in] args Variable argument list
//!
//! @return 0 on success, non-zero error code on failure
int memfault_esp_port_vprintf_log_hook(const char *fmt, va_list args);

//! Initializes the Memfault system, and should be called one time by the application during boot.
void memfault_boot(void);

#ifdef __cplusplus
}
#endif
