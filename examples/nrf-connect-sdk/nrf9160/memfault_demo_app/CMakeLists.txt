# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

if(DEFINED ENV{MEMFAULT_ZEPHYR_EXTRA_MODULE_INCLUDE})
  list(APPEND ZEPHYR_EXTRA_MODULES $ENV{ZEPHYR_BASE}/../modules/memfault-firmware-sdk/ports)
endif()

# Required for app to compile against nRF Connect SDK <= v1.2
if(DEFINED ENV{MEMFAULT_INCLUDE_ZEPHYR_BOILERPLATE})
  include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
  include($ENV{ZEPHYR_BASE}/../nrf/cmake/boilerplate.cmake)
endif()

# Starting in nRF Connect SDK >= 1.3, versions are exposed in
# a CMake package:
#   https://github.com/nrfconnect/sdk-nrf/blob/master/share/ncs-package/cmake/NcsConfig.cmake
#
# We can pre-load the package before Zephyr to dynamically change Kconfig options based on version
# which is not something that can be achieved with Zephyr today
find_package(Ncs HINTS $ENV{ZEPHYR_BASE}/../nrf)

# Conditionally set Kconfig variables based on nRF Connect SDK version
# (since setting non-existent variables result in a build time error which
# would break compiling older builds)
#
# Zephyrs Kconfig extension pulls variables via the "CMake variable cache"
# so we set the ones we need here using that:
#   https://docs.zephyrproject.org/latest/guides/build/kconfig/setting.html#the-initial-configuration
if (NCS_VERSION_MAJOR)
  if (${NCS_VERSION_MAJOR} LESS_EQUAL 1 AND ${NCS_VERSION_MINOR} LESS 6 )
    # Required for logging to work from crash, deprecated in Zephyr included in NCS 1.6
    set(CONFIG_LOG_IMMEDIATE y CACHE INTERNAL "")
  elseif (${NCS_VERSION_MAJOR} LESS_EQUAL 1 AND ${NCS_VERSION_MINOR} GREATER_EQUAL 6 )
    set(CONFIG_MEMFAULT_NCS_PROVISION_CERTIFICATES n CACHE INTERNAL "")
    set(CONFIG_MEMFAULT_NCS_DEVICE_ID_RUNTIME y CACHE INTERNAL "")

    set(CONFIG_NEWLIB_LIBC y CACHE INTERNAL "")
  endif()

  # These were removed in NCS 1.9, but are needed prior to that.
  if (${NCS_VERSION_MAJOR} LESS_EQUAL 1 AND ${NCS_VERSION_MINOR} LESS 9 AND ${NCS_VERSION_PATCH} LESS 99)
    set(CONFIG_BSD_LIBRARY y CACHE INTERNAL "")
    set(CONFIG_BSD_LIBRARY_SYS_INIT n CACHE INTERNAL "")
    # ^ Note: CONFIG_BSD_ were renamed to _NRF_MODEM_ in
    # nRF Connect SDK v1.5. We use the legacy names for now
    # so this app can continue to compile on older targets
    #
    # CONFIG_NRF_MODEM_LIB
    # CONFIG_NRF_MODEM_LIB_SYS_INIT

    # For nRF Connect SDK v1.5 default value changed here
    set(CONFIG_AT_CMD_SYS_INIT n CACHE INTERNAL "")
  else()
    # in v1.9+, use the modem_info library to get the IMEI for device_serial
    set(CONFIG_MODEM_INFO y CACHE INTERNAL "")
  endif()
endif()

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/watchdog.c)
zephyr_include_directories(config)
