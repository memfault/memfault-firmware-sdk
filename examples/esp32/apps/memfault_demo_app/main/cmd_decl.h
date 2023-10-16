/* Console example — declarations of command registration functions.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Register system console commands
void register_system(void);

// Register WiFi console commands
void register_wifi(void);

// Attempt to join a wifi network
bool wifi_join(const char* ssid, const char* pass);
void wifi_load_creds(char** ssid, char** password);

#define MEMFAULT_PROJECT_KEY_LEN 32
// if gcc < 11, disable -Wattributes, "access" attribute is not supported
#if __GNUC__ < 11
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wattributes"
#endif
__attribute__((access(write_only, 1, 2))) int wifi_get_project_key(char* project_key,
                                                                   size_t project_key_len);
#if __GNUC__ < 11
  #pragma GCC diagnostic pop
#endif
// Register app-specific console commands
void register_app(void);

// This semaphore is used to control the Task Watchdog example task
extern SemaphoreHandle_t g_example_task_lock;
