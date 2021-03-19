# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

list(APPEND ZEPHYR_EXTRA_MODULES $ENV{ZEPHYR_BASE}/../modules/memfault-firmware-sdk/ports/nrf-connect-sdk)

# Required for app to compile against nRF Connect SDK <= v1.2
if(DEFINED ENV{MEMFAULT_INCLUDE_ZEPHYR_BOILERPLATE})
  include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
  include($ENV{ZEPHYR_BASE}/../nrf/cmake/boilerplate.cmake)
endif()

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/watchdog.c)
zephyr_include_directories(config)
