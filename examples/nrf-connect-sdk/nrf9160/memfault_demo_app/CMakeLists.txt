# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

if(DEFINED ENV{MEMFAULT_ZEPHYR_EXTRA_MODULE_INCLUDE})
  list(APPEND ZEPHYR_EXTRA_MODULES $ENV{ZEPHYR_BASE}/../modules/memfault-firmware-sdk/ports)
endif()

# Starting in nRF Connect SDK >= 1.3, versions are exposed in
# a CMake package:
#   https://github.com/nrfconnect/sdk-nrf/blob/master/share/ncs-package/cmake/NcsConfig.cmake
#
# We can pre-load the package before Zephyr to dynamically change Kconfig options based on version
# which is not something that can be achieved with Zephyr today
find_package(Ncs HINTS $ENV{ZEPHYR_BASE}/../nrf)

# Confirm NCS_VERSION_MAJOR is defined
if (NOT DEFINED NCS_VERSION_MAJOR)
  message(FATAL_ERROR "NCS Version not found, please contact Memfault support!")
endif()

# Below we conditionally set Kconfig variables via overlays based on nRF Connect SDK version
# (since setting non-existent variables result in a build time error which
# would break compiling older builds)

if (${NCS_VERSION_MAJOR}.${NCS_VERSION_MINOR}.${NCS_VERSION_PATCH} VERSION_GREATER_EQUAL 2.9.99)
set(OVERLAY_CONFIG "${OVERLAY_CONFIG};overlays/ncs-main.conf")
endif()

if (${NCS_VERSION_MAJOR}.${NCS_VERSION_MINOR}.${NCS_VERSION_PATCH} VERSION_LESS 2.9.99)
set(OVERLAY_CONFIG "${OVERLAY_CONFIG};overlays/ncs-pre-v2.9.99.conf")
endif()

if (${NCS_VERSION_MAJOR}.${NCS_VERSION_MINOR}.${NCS_VERSION_PATCH} VERSION_LESS 2.8.0)
set(OVERLAY_CONFIG "${OVERLAY_CONFIG};overlays/ncs-pre-v2.8.0.conf")
endif()

if (${NCS_VERSION_MAJOR}.${NCS_VERSION_MINOR}.${NCS_VERSION_PATCH} VERSION_LESS 2.2.99)
set(OVERLAY_CONFIG "${OVERLAY_CONFIG};overlays/ncs-pre-v2.2.99.conf")
endif()

if (${NCS_VERSION_MAJOR}.${NCS_VERSION_MINOR}.${NCS_VERSION_PATCH} VERSION_LESS 2.4.0)
set(OVERLAY_CONFIG "${OVERLAY_CONFIG};overlays/ncs-pre-v2.4.0.conf")
endif()

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/watchdog.c)
zephyr_include_directories(config)
