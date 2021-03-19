cmake_minimum_required(VERSION 3.13.1)

# The Zephyr port to target (i.e "v1.14", "v2.0", "v2.4")
if(NOT DEFINED ENV{MEMFAULT_ZEPHYR_PORT_TARGET})
  message(FATAL_ERROR "MEMFAULT_ZEPHYR_PORT_TARGET is not in env")
endif()

# The path to the memfault-firmware-sdk
if(NOT DEFINED ENV{MEMFAULT_SDK_DIR})
  message(FATAL_ERROR "MEMFAULT_SDK_DIR is not in env")
endif()

# Add the memfault-firmware-sdk Zephyr port to the project
# NB: CONFIG_MEMFAULT=y will also need to be set in prj.conf to enable
set(MEMFAULT_ZEPHYR_PORT_TARGET $ENV{MEMFAULT_ZEPHYR_PORT_TARGET})
list(APPEND ZEPHYR_EXTRA_MODULES $ENV{MEMFAULT_SDK_DIR}/ports)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(memfault_demo_app)

zephyr_include_directories(config)
target_sources(app PRIVATE src/main.c)
