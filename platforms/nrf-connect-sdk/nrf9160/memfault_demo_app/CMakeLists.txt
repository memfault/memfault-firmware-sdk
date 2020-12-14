# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

list(APPEND ZEPHYR_EXTRA_MODULES $ENV{ZEPHYR_BASE}/../modules/memfault-firmware-sdk/ports/nrf-connect-sdk)
set(spm_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/spm.conf)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

target_sources(app PRIVATE src/main.c)
zephyr_include_directories(config)
