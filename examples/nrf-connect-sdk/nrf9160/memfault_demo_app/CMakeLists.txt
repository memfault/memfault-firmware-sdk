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

if (NOT NCS_VERSION_MAJOR)
  # Note: If we cannot resolve the NCS version, assume we are using 1.2.x which is the oldest
  # version Memfault has been tested against
  set(NCS_VERSION_MAJOR 1)
  set(NCS_VERSION_MINOR 2)
  set(NCS_VERSION_PATCH 0)
endif()

# Below we conditionally set Kconfig variables based on nRF Connect SDK version
# (since setting non-existent variables result in a build time error which
# would break compiling older builds)
#
# Zephyrs Kconfig extension pulls variables via the "CMake variable cache"
# so we set the ones we need here using that:
#   https://docs.zephyrproject.org/latest/build/kconfig/setting.html#the-initial-configuration

if (${NCS_VERSION_MAJOR}.${NCS_VERSION_MINOR}.${NCS_VERSION_PATCH} VERSION_LESS 1.3)
else() # nRF Connect SDK Version >= 1.3.0
  # We cannot set for NCS <= 1.2 because on older versions of zephyr the argument was not
  # directly user-configurable (no prompt in Kconfig)
  set(CONFIG_SHELL_LOG_BACKEND n CACHE INTERNAL "")
endif()

if (${NCS_VERSION_MAJOR}.${NCS_VERSION_MINOR}.${NCS_VERSION_PATCH} VERSION_LESS 1.4)
else() # nRF Connect SDK Version >= 1.4.0
  # We need the response for a FOTA download request to fit within a 2kB buffer (space nRF91 modem
  # allocates). This parameter bounds this size by controlling the maximum amount of data we
  # request in an HTTP get range request.
  set(CONFIG_DOWNLOAD_CLIENT_HTTP_FRAG_SIZE_1024 y CACHE INTERNAL "")
endif()

if (${NCS_VERSION_MAJOR}.${NCS_VERSION_MINOR}.${NCS_VERSION_PATCH} VERSION_LESS 1.5)
  # Legacy name for CONFIG_DEBUG_THREAD_INFO needed for nRF Connect version <= 1.5
  # due to the version of Zephyr included (pre v2.5)
  set(CONFIG_OPENOCD_SUPPORT y CACHE INTERNAL "")
else() # nRF Connect SDK Version >= 1.5.0
  # Enable Memfault FOTA Support
  #
  # We enable conditionally because prior to NCS < 1.5 a small patch (https://mflt.io/nrf-fota) was
  # needed in order for things to work
  set(CONFIG_MEMFAULT_FOTA y CACHE INTERNAL "")
  set(CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE 400 CACHE INTERNAL "")
  set(CONFIG_DOWNLOAD_CLIENT_STACK_SIZE 1600 CACHE INTERNAL "")

  # Note: Can optionally be changed to =y to implement
  # a custom event handler for FOTA events
  # CONFIG_MEMFAULT_FOTA_DOWNLOAD_CALLBACK_CUSTOM=n
endif()


if (${NCS_VERSION_MAJOR}.${NCS_VERSION_MINOR}.${NCS_VERSION_PATCH} VERSION_LESS 1.6.0)
  # Required for logging to work from crash but deprecated in Zephyr included in NCS 1.6
  set(CONFIG_LOG_IMMEDIATE y CACHE INTERNAL "")

  # NCS 1.6 was the first one which introduced setting the project key via
  # Kconfig. Look for this argument and remap to a regular compiler definition
  # that gets picked up in memfault_demo_app/src/main.c
  if (CONFIG_MEMFAULT_NCS_PROJECT_KEY)
    add_compile_definitions(MEMFAULT_NCS_PROJECT_KEY=${CONFIG_MEMFAULT_NCS_PROJECT_KEY})
    unset(CONFIG_MEMFAULT_NCS_PROJECT_KEY CACHE)
  endif()
else() # nRF Connect SDK Version >= 1.6.0
  set(CONFIG_MEMFAULT_NCS_PROVISION_CERTIFICATES n CACHE INTERNAL "")
  set(CONFIG_MEMFAULT_NCS_DEVICE_ID_RUNTIME y CACHE INTERNAL "")
  set(CONFIG_NEWLIB_LIBC y CACHE INTERNAL "")
endif()

if (${NCS_VERSION_MAJOR}.${NCS_VERSION_MINOR}.${NCS_VERSION_PATCH} VERSION_LESS 1.7.99)
  # These were removed in NCS 1.8.0, but are needed prior to that. Enable them
  # if NCS version <1.7.99 (.99 patch version is used for the next revision's
  # development series)
  set(CONFIG_BSD_LIBRARY y CACHE INTERNAL "")
  set(CONFIG_BSD_LIBRARY_SYS_INIT n CACHE INTERNAL "")
  # ^ Note: CONFIG_BSD_ were renamed to _NRF_MODEM_ in
  # nRF Connect SDK v1.5. We use the legacy names here
  # so the app can continue to compile on older targets
  #
  # CONFIG_NRF_MODEM_LIB
  # CONFIG_NRF_MODEM_LIB_SYS_INIT

  # For nRF Connect SDK v1.5 default value changed here
  set(CONFIG_AT_CMD_SYS_INIT n CACHE INTERNAL "")
else() # nRF Connect SDK Version >= 1.8.0
  # in v1.8+, use the modem_info library to get the IMEI for device_serial
  set(CONFIG_MODEM_INFO y CACHE INTERNAL "")

  # pre-2.3.0, SPM (Secure Partition Manager) can be used instead of the default
  # TFM, and results in nicer backtraces in certain cases, so select it here.
  if (${NCS_VERSION_MAJOR}.${NCS_VERSION_MINOR}.${NCS_VERSION_PATCH} VERSION_LESS 2.2.99)
    # Use SPM instead of TFM for Secure Firmware as TFM does not (yet) support forwarding of fault handlers
    set(CONFIG_BUILD_WITH_TFM n CACHE INTERNAL "")
    # Explicitly set SPM=y because as of nRF Connect >= 2.1.0 it is marked as deprecated and no longer enabled by default
    set(CONFIG_SPM y CACHE INTERNAL "")
  endif()
endif()

if (${NCS_VERSION_MAJOR}.${NCS_VERSION_MINOR}.${NCS_VERSION_PATCH} GREATER_EQUAL 2.4.0)
  # In Zephyr v3.4 release, FLASH_MAP and STREAM_FLASH must be specified manually
  #  https://github.com/zephyrproject-rtos/zephyr/commit/c3bfe7a6baf7e47d8ad70e38ffdea313f3ec62ce
  #
  # These are required for NCS >= 2.4
  set(CONFIG_FLASH_MAP y CACHE INTERNAL "")
  set(CONFIG_STREAM_FLASH y CACHE INTERNAL "")

  set(CONFIG_AT_SHELL y CACHE INTERNAL "")
  # Make it possible to send AT commands ending with a "?"
  #  https://developer.nordicsemi.com/nRF_Connect_SDK/doc/2.4.2/nrf/libraries/modem/at_shell.html
  set(CONFIG_SHELL_WILDCARD n CACHE INTERNAL "")
endif()

# Required for app to compile against nRF Connect SDK <= v1.2
# Must be included after updating Kconfig settings in CMake Cache above
if(DEFINED ENV{MEMFAULT_INCLUDE_ZEPHYR_BOILERPLATE})
  include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
  include($ENV{ZEPHYR_BASE}/../nrf/cmake/boilerplate.cmake)
endif()

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/watchdog.c)
zephyr_include_directories(config)
