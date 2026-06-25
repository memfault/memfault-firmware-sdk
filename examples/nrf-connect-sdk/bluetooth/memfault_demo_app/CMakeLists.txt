# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# The default board for this example is the nrf52840dk/nrf52840, but can be
# overridden by passing --board=<board> to west or -DBOARD=<board> to cmake
set(BOARD nrf52840dk/nrf52840)

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

if (${NCS_VERSION_MAJOR}.${NCS_VERSION_MINOR}.${NCS_VERSION_PATCH} VERSION_LESS 3.2.0)
set(OVERLAY_CONFIG "${OVERLAY_CONFIG};overlays/ncs-pre-v3.2.0.conf")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(fs_shell)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE
  src/main.c
  src/shell_commands.c
  src/bt_smp_dfu_session.c
)

zephyr_include_directories(config)

# bt_smp_dfu_session.c includes <.../img_mgmt/img_mgmt.h>, which pulls in
# mcuboot's bootutil headers. Link the bootutil interface library to inherit its
# include paths (mcuboot_config/, bootutil/). Guarded so the example still
# configures on builds without MCUboot.
if(TARGET MCUBOOT_BOOTUTIL)
  target_link_libraries(app PRIVATE MCUBOOT_BOOTUTIL)
endif()

# Normalize dbug + __FILE__ paths to relative locations:
# 1. west workspace
# 2. toolchain install location (applies to paths in cross tool library files)
#
# If debugging from location 1., the west-manifest files should already have the
# correct relative path for GDB to locate them, otherwise the GDB "directory"
# command can be used to tell GDB where to find the source files (also required
# if the cross tool libraries/headers need to be located by GDB)
get_filename_component(WEST_PROJECT_ROOT_PATH
                       "${CMAKE_CURRENT_LIST_DIR}/.."
                       ABSOLUTE)
zephyr_compile_options(
    -ffile-prefix-map=${WEST_PROJECT_ROOT_PATH}=.
    -ffile-prefix-map=${TOOLCHAIN_HOME}=.
    -Wa,--debug-prefix-map=${WEST_PROJECT_ROOT_PATH}=.
)

# If building for nrf54l15dk, add a custom linker fragment for placing the
# noinit section.
if(CONFIG_BOARD_NRF54L15DK)
  zephyr_linker_sources(SECTIONS
    ${CMAKE_CURRENT_SOURCE_DIR}/boards/nrf54l15dk_nrf54l15_cpuapp_noinit.ld
  )
endif()

# Same for nrf54lm20DK
if(CONFIG_BOARD_NRF54LM20DK)
  zephyr_linker_sources(SECTIONS
    ${CMAKE_CURRENT_SOURCE_DIR}/boards/nrf54lm20dk_nrf54lm20a_cpuapp_noinit.ld
  )
endif()
