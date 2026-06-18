# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

if(DEFINED ENV{MEMFAULT_ZEPHYR_EXTRA_MODULE_INCLUDE})
  list(APPEND ZEPHYR_EXTRA_MODULES $ENV{ZEPHYR_BASE}/../modules/memfault-firmware-sdk/ports)
endif()

# We need the nRF Connect SDK version before loading Zephyr, to dynamically
# select Kconfig overlays based on version- not something that can be achieved
# with Zephyr today.
#
# Note: previously this was done with 'find_package(Ncs)', but as of sdk-nrf
# commit 40f7487d3a ("cmake: adopt VERSION to common VERSION file format"),
# NcsConfig.cmake requires ZEPHYR_BASE to be set, which is not available before
# 'find_package(Zephyr)'. Instead, read the version directly from the VERSION
# file, supporting both formats:
# - nRF Connect SDK <= v3.3.0: single line, "<major>.<minor>.<patch>[-extra]"
# - newer: Zephyr-style "VERSION_MAJOR = <major>" etc. key-value lines
set(NCS_VERSION_FILE "$ENV{ZEPHYR_BASE}/../nrf/VERSION")
if(EXISTS "${NCS_VERSION_FILE}")
  file(READ "${NCS_VERSION_FILE}" ncs_version_content)
  if(ncs_version_content MATCHES "VERSION_MAJOR *= *([0-9]+)")
    set(NCS_VERSION_MAJOR ${CMAKE_MATCH_1})
    string(REGEX MATCH "VERSION_MINOR *= *([0-9]+)" _ "${ncs_version_content}")
    set(NCS_VERSION_MINOR ${CMAKE_MATCH_1})
    string(REGEX MATCH "PATCHLEVEL *= *([0-9]+)" _ "${ncs_version_content}")
    set(NCS_VERSION_PATCH ${CMAKE_MATCH_1})
  elseif(ncs_version_content MATCHES "([0-9]+)\\.([0-9]+)\\.([0-9]+)")
    set(NCS_VERSION_MAJOR ${CMAKE_MATCH_1})
    set(NCS_VERSION_MINOR ${CMAKE_MATCH_2})
    set(NCS_VERSION_PATCH ${CMAKE_MATCH_3})
  endif()
endif()

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

if (${NCS_VERSION_MAJOR}.${NCS_VERSION_MINOR}.${NCS_VERSION_PATCH} VERSION_GREATER_EQUAL 3.3.0)
set(OVERLAY_CONFIG "${OVERLAY_CONFIG};overlays/ncs-v3.3.0.conf")
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

if (${NCS_VERSION_MAJOR}.${NCS_VERSION_MINOR}.${NCS_VERSION_PATCH} VERSION_LESS 2.0.0)
set(OVERLAY_CONFIG "${OVERLAY_CONFIG};overlays/ncs-pre-v2.0.0.conf")
endif()

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/watchdog.c)
zephyr_include_directories(config)
