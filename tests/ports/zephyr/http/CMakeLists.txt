# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Use the local memfault-firmware-sdk from this repository instead of cloning
# it via west.
if(NOT DEFINED ENV{MEMFAULT_SDK_DIR})
    set(ENV{MEMFAULT_SDK_DIR} ${CMAKE_CURRENT_LIST_DIR}/../../../../)
endif()
list(APPEND ZEPHYR_EXTRA_MODULES $ENV{MEMFAULT_SDK_DIR}/ports)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test-app)

target_sources(app PRIVATE
    src/main.c
    src/test_platform_http.c
    src/test_http_response_codes.c
)
