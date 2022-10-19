cmake_minimum_required(VERSION 3.13.1)

# The Zephyr port to target (i.e "v2.0", "v2.4")
if(NOT DEFINED ENV{MEMFAULT_ZEPHYR_PORT_TARGET})
  # Lar
  set(ENV{MEMFAULT_ZEPHYR_PORT_TARGET} v2.4)
endif()

# The path to the memfault-firmware-sdk
if(NOT DEFINED ENV{MEMFAULT_SDK_DIR})
  set(ENV{MEMFAULT_SDK_DIR} ${CMAKE_CURRENT_LIST_DIR}/../../../..)
endif()

# Add the memfault-firmware-sdk Zephyr port to the project
# NB: CONFIG_MEMFAULT=y will also need to be set in prj.conf to enable
set(MEMFAULT_ZEPHYR_PORT_TARGET $ENV{MEMFAULT_ZEPHYR_PORT_TARGET})
list(APPEND ZEPHYR_EXTRA_MODULES $ENV{MEMFAULT_SDK_DIR}/ports)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(memfault_demo_app)

zephyr_include_directories(config)
target_sources(app PRIVATE src/main.c)
