//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//!
//! A simple test app which can be used for testing the Particle Memfault library

#include "Particle.h"
#include "application.h"
#include "spark_wiring_json.h"

#include "memfault.h"

#include "logging.h"
LOG_SOURCE_CATEGORY("memfault_app")

SYSTEM_MODE(MANUAL);

//! Note: Must be bumped everytime modifications are made
//!   https://mflt.io/particle-versioning
#define EXAMPLE_APP_VERSION 1

PRODUCT_ID(PLATFORM_ID);
PRODUCT_VERSION(EXAMPLE_APP_VERSION)

// Enable USB logging and enable full verbosity for debug purposes
static SerialLogHandler s_usb_log_handler(115200, LOG_LEVEL_ALL);


// Note: For test purposes, we defer initializing the Memfault library to
// give time for the USB serial to be initialized and console logs to be up and running
//
// For a production application, we recommend constructing Memfault on bootup by
// adding the following to your application:
//
// static Memfault s_memfault(your_product_id, your_product_version);
// void loop() {
//   [...]
//   memfault.process();
// }
static Memfault *s_memfault = NULL;

#if Wiring_WiFi
#error "Support for WiFi transport not implemented yet"
#endif

void setup() {
  Particle.connect();
}

//! Note: This function loops forever
void loop() {
  if (s_memfault == NULL) {
    // insert a delay to give Device OS USB init time to complete
    delay(1000);
    s_memfault = new Memfault(EXAMPLE_APP_VERSION);
  }

  s_memfault->process();
  Particle.process();
}

JSONValue getValue(const JSONValue& obj, const char* name) {
  JSONObjectIterator it(obj);
  while (it.next()) {
    if (it.name() == name) {
      return it.value();
    }
  }
  return JSONValue();
}

void ctrl_request_custom_handler(ctrl_request* req) {
  LOG(INFO, "Received Command: %.*s", req->request_size, req->request_data);

  auto d = JSONValue::parse(req->request_data, req->request_size);
  SPARK_ASSERT(d.isObject());
  JSONValue data_ = std::move(d);

  const char *command = getValue(data_, "command").toString().data();

  //! No support for argv / argc forwarding today
  const bool success = s_memfault->run_debug_cli_command(command, 0, NULL);
  system_ctrl_set_result(req, success ? SYSTEM_ERROR_NONE : SYSTEM_ERROR_NOT_SUPPORTED, nullptr, nullptr, nullptr);
}
