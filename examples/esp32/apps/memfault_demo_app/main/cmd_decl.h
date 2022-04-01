/* Console example — declarations of command registration functions.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Register system functions
void register_system(void);

// Register WiFi functions
void register_wifi(void);

// Register app-specific functions

// This semaphore is used to control the Task Watchdog example task
extern SemaphoreHandle_t g_example_task_lock;

void register_app(void);
