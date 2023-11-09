if (CONFIG_MEMFAULT_AUTOMATIC_INIT)
  message(FATAL_ERROR "CONFIG_MEMFAULT_AUTOMATIC_INIT has been deprecated.
    Please complete the following steps:
    1. Remove CONFIG_MEMFAULT_AUTOMATIC_INIT=y from sdkconfig.default if present
    2. Delete your project's generated sdkconfig (be sure to save any in-progress changes)
    3. Update your application to call memfault_boot during initialization
    For more information please see https://docs.memfault.com/docs/mcu/esp32-guide")
endif()

set(MEMFAULT_SDK_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../..)

include(${MEMFAULT_SDK_ROOT}/cmake/Memfault.cmake)

# Select RISCV or XTENSA architecture, depending on target chip
if(DEFINED IDF_VERSION_MAJOR)
  # idf_build_get_property was added in 4.0. RISC-V chips weren't added until
  # v4.3, but we can rely on the build target for figuring out the architecture.
  if(IDF_VERSION_MAJOR VERSION_GREATER_EQUAL 4)
    # Get target architecture to pass to the memfault_library initialization.
    # IDF_TARGET_ARCH was added in 5.0, so use the older IDF_TARGET variable.
    idf_build_get_property(target IDF_TARGET)
    if("${target}" STREQUAL "esp32" OR "${target}" STREQUAL "esp32s2" OR "${target}" STREQUAL "esp32s3")
      set(ESP_ARCH "ARCH_XTENSA")
    else()
      set(ESP_ARCH "ARCH_RISCV")
    endif()
  else()
    # For older versions of esp-idf, we assume the architecture is XTENSA
    set(ESP_ARCH "ARCH_XTENSA")
  endif()
else()
  # If the IDF_VERSION_MAJOR variable is not defined, we assume the architecture is XTENSA- this is
  # a pretty old version of esp-idf, much earlier than when RISC-V support was added.
  set(ESP_ARCH "ARCH_XTENSA")
endif()

list(APPEND MEMFAULT_COMPONENTS core util panics demo http metrics)
memfault_library(${MEMFAULT_SDK_ROOT} MEMFAULT_COMPONENTS
  MEMFAULT_COMPONENTS_SRCS MEMFAULT_COMPONENTS_INC_FOLDERS ${ESP_ARCH})

# v4.0 and greater expose the IDF_VERSION in cmake. If we can't find
# it, we assume the end user is on v3.x

include($ENV{IDF_PATH}/tools/cmake/version.cmake OPTIONAL)

# Version 3.3.5 and later provide IDF_VERSION_MAJOR.
if(DEFINED IDF_VERSION_MAJOR)
  if(IDF_VERSION_MAJOR VERSION_EQUAL 5)
    set(MEMFAULT_ESP_IDF_PORT v5.x)
  elseif(IDF_VERSION_MAJOR VERSION_EQUAL 4)
    set(MEMFAULT_ESP_IDF_PORT v4.x)
  elseif(IDF_VERSION_MAJOR VERSION_EQUAL 3)
    set(MEMFAULT_ESP_IDF_PORT v3.x)
  else()
    message(FATAL_ERROR "IDF_VERSION_MAJOR=${IDF_VERSION_MAJOR} provided is not currenly supported")
  endif()
else()
  message(STATUS "No IDF_VERSION_MAJOR provided, using esp-idf v3.x Memfault port")
  set(MEMFAULT_ESP_IDF_PORT v3.x)
endif()

# esp-idf version specific porting files
list(APPEND MEMFAULT_COMPONENTS_SRCS
  ${MEMFAULT_ESP_IDF_PORT}/memfault_esp_spi_flash.c
)

include(${CMAKE_CURRENT_LIST_DIR}/${MEMFAULT_ESP_IDF_PORT}/Memfault-esp-idf-compat.cmake)

# This directory holds ports that are common across v3.x and v4.x esp-idf releases
set(MEMFAULT_ESP_IDF_PORT_COMMON common)
list(APPEND MEMFAULT_COMPONENTS_SRCS
    ${MEMFAULT_ESP_IDF_PORT_COMMON}/memfault_fault_handler.c
    ${MEMFAULT_ESP_IDF_PORT_COMMON}/memfault_platform_core.c
    ${MEMFAULT_ESP_IDF_PORT_COMMON}/memfault_platform_coredump.c
    ${MEMFAULT_ESP_IDF_PORT_COMMON}/memfault_platform_debug_log.c
    ${MEMFAULT_ESP_IDF_PORT_COMMON}/memfault_platform_demo_cli_cmds.c
    ${MEMFAULT_ESP_IDF_PORT_COMMON}/memfault_platform_device_info.c
    ${MEMFAULT_ESP_IDF_PORT_COMMON}/memfault_platform_http_client_buffer.c
    ${MEMFAULT_ESP_IDF_PORT_COMMON}/memfault_platform_http_client.c
)

# Add a legacy metrics file for now. To be removed in the near future when v3.x
# support is dropped.
if(IDF_VERSION_MAJOR VERSION_EQUAL 3)
  list(APPEND MEMFAULT_COMPONENTS_SRCS
    ${MEMFAULT_ESP_IDF_PORT}/memfault_platform_metrics_legacy.c
  )
else()
  list(APPEND MEMFAULT_COMPONENTS_SRCS
    ${MEMFAULT_ESP_IDF_PORT_COMMON}/memfault_platform_metrics.c
  )
endif()

list(APPEND MEMFAULT_COMPONENTS_INC_FOLDERS
   include
   include/${MEMFAULT_ESP_IDF_PORT}
   ${MEMFAULT_SDK_ROOT}/ports/include
   $ENV{MEMFAULT_PLATFORM_EXTRA_INCLUDES}
   config
)

if (CONFIG_MEMFAULT_LWIP_METRICS)
  list(APPEND MEMFAULT_COMPONENTS_SRCS ${MEMFAULT_SDK_ROOT}/ports/lwip/memfault_lwip_metrics.c)
  list(APPEND MEMFAULT_COMPONENTS_INC_FOLDERS ${MEMFAULT_SDK_ROOT}/ports/lwip/config)
endif()

if (CONFIG_MEMFAULT_FREERTOS_TASK_RUNTIME_STATS)
  # if CONFIG_FREERTOS_RUN_TIME_STATS_USING_ESP_TIMER is not set, generate an error
  # message and exit
  if(NOT CONFIG_FREERTOS_RUN_TIME_STATS_USING_ESP_TIMER)
    message(FATAL_ERROR "CONFIG_FREERTOS_RUN_TIME_STATS_USING_ESP_TIMER is not set. Please set it to 'y' to use FreeRTOS task runtime stats")
  endif()
  list(APPEND MEMFAULT_COMPONENTS_SRCS ${MEMFAULT_SDK_ROOT}/ports/freertos/src/memfault_sdk_metrics_freertos.c)
  list(APPEND MEMFAULT_COMPONENTS_INC_FOLDERS ${MEMFAULT_SDK_ROOT}/ports/freertos/config)
endif()

if (CONFIG_MEMFAULT_MBEDTLS_METRICS)
  list(APPEND MEMFAULT_COMPONENTS_SRCS ${MEMFAULT_SDK_ROOT}/ports/mbedtls/memfault_mbedtls_metrics.c)
  list(APPEND MEMFAULT_COMPONENTS_INC_FOLDERS ${MEMFAULT_SDK_ROOT}/ports/mbedtls/config)
endif()

# For version >= 4.4.3, we can collect smaller coredumps by default
# by prioritizing active stack and FreeRTOS regions first. ESP-IDF < 4.4.3
# uses a simpler scheme collecting all of DRAM. See
# common/memfault_platform_coredump.c for more info.
# Note: CMake does not short-circuit logic statements, nested ifs required
# Note: ENV{IDF_VERSION} added in esp-idf 4.4.3
if (DEFINED ENV{IDF_VERSION})
  if ($ENV{IDF_VERSION} VERSION_GREATER_EQUAL "4.4.3")
    list(APPEND MEMFAULT_COMPONENTS_SRCS
      ${MEMFAULT_SDK_ROOT}/ports/freertos/src/memfault_freertos_ram_regions.c
    )

    # Add a linker fragment to place FreeRTOS timers and task objects in the same area of dram0.bss
    set(COMPONENT_ADD_LDFRAGMENTS "${MEMFAULT_ESP_IDF_PORT_COMMON}/memfault_esp_freertos.lf")
  endif()
endif()


# Register Memfault SDK Component
set(COMPONENT_SRCS ${MEMFAULT_COMPONENTS_SRCS})
set(COMPONENT_ADD_INCLUDEDIRS ${MEMFAULT_COMPONENTS_INC_FOLDERS})
list(APPEND COMPONENT_REQUIRES
    ${MEMFAULT_ESP_IDF_VERSION_SPECIFIC_REQUIRES}
    $ENV{MEMFAULT_PLATFORM_PORT_COMPONENTS}
    freertos
    heap
    log
    soc
    spi_flash
    console
    driver
)

if(CONFIG_MEMFAULT_LWIP_METRICS)
  list(APPEND COMPONENT_REQUIRES lwip)
endif()
if(CONFIG_MEMFAULT_MBEDTLS_METRICS)
  list(APPEND COMPONENT_REQUIRES mbedtls)
endif()

if($ENV{MEMFAULT_ESP_HTTP_CLIENT_ENABLE})
  list(APPEND COMPONENT_REQUIRES esp_http_client esp_https_ota)
endif()
register_component()

# sdk_overrides/esp_http_client.c needs the (private) esp_http header files:
get_filename_component(this_directory_name . ABSOLUTE DIRECTORY)
get_filename_component(this_directory_name ${this_directory_name} NAME)
mflt_esp32_component_get_target(this_component ${this_directory_name})

# The below compilation options need to be set after register_component().
if (CONFIG_MEMFAULT_FREERTOS_TASK_RUNTIME_STATS)
  # MEMFAULT_USE_NEW_FREERTOS_IDLETASK_RUNTIME_API should be explicitly set to 1
  # for ESP-IDF. The API in question, 'ulTaskGetIdleRunTimeCounter', was added
  # in ESP-IDF v4.3, but didn't have backwards compatibility with the original
  # name 'xTaskGetIdleRunTimeCounter' until ESP-IDF v4.4 (because ESP-IDF pulled
  # a non-release upstream FreeRTOS in the interim). The old name
  # 'xTaskGetIdleRunTimeCounter' was never used in any version of ESP-IDF.
  #
  # 'xTaskGetIdleRunTimeCounter' was added in FreeRTOS V10.2.0, renamed to
  # 'ulTaskGetIdleRunTimeCounter'in V10.3.0, with backwards compatibility
  # included in mainline FreeRTOS V10.3.0.
  #
  # Task runtime stats are disabled on FreeRTOS <10.2.0 entirely, so on earlier
  # ESP-IDF SDK versions, this is unused.
  component_compile_options(-DMEMFAULT_USE_NEW_FREERTOS_IDLETASK_RUNTIME_API=1)
endif()

component_compile_options(-DMEMFAULT_ESP_HTTP_CLIENT_ENABLE=$ENV{MEMFAULT_ESP_HTTP_CLIENT_ENABLE})

# FreeRTOS error logging redirect for ESP-IDF compatibility.
component_compile_options(
  -DMEMFAULT_FREERTOS_REGISTRY_FULL_ERROR_LOG_INCLUDE="memfault_platform_freertos_error_log.h"
)

# Set the heartbeat config file to use the ESP-IDF port file, which will bring
# in the user's heartbeat config automatically. Set it as a global compiler
# option so it properly affects both component compilation and when the metric
# keys are used in the application.
#
# Set the ESP-IDF specific Memfault platform config header as well.
if (IDF_VERSION_MAJOR VERSION_EQUAL 3)
  # On pre-v4 of ESP-IDF, insert the compiler flag to the global CMAKE_C_FLAGS
  set(
    memfault_c_flags
    "-DMEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE=\\\"memfault_esp_metrics_heartbeat_config.def\\\""
    "-DMEMFAULT_PLATFORM_CONFIG_FILE=\\\"memfault_esp_idf_port_config.h\\\""
    "-DMEMFAULT_TRACE_REASON_USER_DEFS_FILE=\\\"memfault_trace_reason_esp_idf_port_config.def\\\""
  )
  # convert from list to string before setting
  list(JOIN memfault_c_flags " " memfault_c_flags)
  set(CMAKE_C_FLAGS
  "${CMAKE_C_FLAGS} ${memfault_c_flags}"
  CACHE STRING
  "Global C Flags"
  FORCE
  )
else()
  # set these as compile_options, not compile_definitions; pre-v5 of ESP-IDF
  # required the -D prefix, post-v5 auto strips it, but for safety, set them as
  # plain options not "definitions"
  list(APPEND compile_options
    "-DMEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE=\"memfault_esp_metrics_heartbeat_config.def\""
    "-DMEMFAULT_PLATFORM_CONFIG_FILE=\"memfault_esp_idf_port_config.h\""
    "-DMEMFAULT_TRACE_REASON_USER_DEFS_FILE=\"memfault_trace_reason_esp_idf_port_config.def\""
  )
  idf_build_set_property(
    COMPILE_OPTIONS
    "${compile_options}"
    APPEND
  )
  # idf_build_get_property(bleh COMPILE_OPTIONS)
  # message(FATAL_ERROR "COMPILE_OPTIONS=${bleh}")
endif()

# We will intercept the panic handlers enabled by CONFIG_ESP32_ENABLE_COREDUMP_TO_FLASH
# and run the Memfault Fault Handler instead.
#
# Benefits here are:
#   FreeRTOS task list is walked server side instead of on device (so you can get crash data even if the lists are corrupted)
#   Much more flexibility in debug information collected (e.g. all RAM, just the current stack trace, select stacks and variables)
#   Data can be posted directly from device to Memfault cloud for deduplication and analysis
target_link_libraries(${this_component} INTERFACE "-Wl,--wrap=esp_core_dump_to_flash -Wl,--wrap=esp_core_dump_init -Wl,--wrap=esp_core_dump_image_get")

# Wrap panic_abort to intercept assert()/abort() calls. Note that support for
# this was added in ESP-IDF v4.2; earlier versions are intercepted with the
# __assert_func definition provided in memfault_stdlib_assert.c
target_link_libraries(${this_component} INTERFACE "-Wl,--wrap=panic_abort")

# Wrap allocator functions to capture mbedTLS metrics
if(CONFIG_MEMFAULT_MBEDTLS_METRICS)
  target_link_libraries(${this_component} INTERFACE "-Wl,--wrap=mbedtls_calloc -Wl,--wrap=mbedtls_free")
endif()

# Include a linker script fragment to support compact logs, when enabled
get_filename_component(compact_log_linker_script ${MEMFAULT_ESP_IDF_PORT_COMMON}/memfault_compact_log.ld ABSOLUTE)
target_link_libraries(
    ${this_component}
    INTERFACE
    -T ${compact_log_linker_script}
)

# Link required libraries and add compiler flags to enable FreeRTOS region collection
# in >= 4.4.3. Note: CMake does not short-circuit logic statements, nested ifs required
# Note: ENV{IDF_VERSION} added in esp-idf 4.4.0
if(DEFINED ENV{IDF_VERSION})
  if ($ENV{IDF_VERSION} VERSION_GREATER_EQUAL "4.4.3")
    # Policy change requires CMake v3.13+
    cmake_minimum_required(VERSION 3.13)

    # First set new policy for target_link_libraries, this resolves a warning when using on
    # targets not created in this directory
    cmake_policy(SET CMP0079 NEW)

    # Get the name of the ESP FreeRTOS target/library
    idf_component_get_property(freertos_lib freertos COMPONENT_LIB)

    # Link this component to FreeRTOS, use INTERFACE because we're only sharing headers
    target_link_libraries(${freertos_lib} INTERFACE ${this_component})

    # Lastly ensure that our FreeRTOS trace hooks are defined first by adding this
    # compile option to the FreeRTOS target to include with all source
    # This method is an alternative to #include within FreeRTOSConfig.h which esp-idf
    # makes very difficult to do.
    get_filename_component(freertos_trace_header ${MEMFAULT_SDK_ROOT}/ports/include/memfault/ports/freertos_trace.h ABSOLUTE)
    target_compile_options(${freertos_lib} INTERFACE
      "-DMEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE=\"memfault_esp_metrics_heartbeat_config.def\""
      "-DMEMFAULT_PLATFORM_CONFIG_FILE=\"memfault_esp_idf_port_config.h\""
      "-DMEMFAULT_TRACE_REASON_USER_DEFS_FILE=\"memfault_trace_reason_esp_idf_port_config.def\""
      -include ${freertos_trace_header}
    )
  endif()
endif()
