# Memfault component inclusion can be disabled by setting the environment
# variable MEMFAULT_DISABLE=1 when building. This can't be done at the Kconfig
# level because we need to load the component before Kconfig runs.
if(DEFINED ENV{MEMFAULT_DISABLE})
  return()
endif()

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

# If $ENV{MEMFAULT_PLATFORM_PORT_COMPONENTS} is set, set that to MEMFAULT_PLATFORM_PORT_COMPONENTS
# otherwise default it to 'main'
if(DEFINED ENV{MEMFAULT_PLATFORM_PORT_COMPONENTS})
  set(MEMFAULT_PLATFORM_PORT_COMPONENTS $ENV{MEMFAULT_PLATFORM_PORT_COMPONENTS})
else()
  set(MEMFAULT_PLATFORM_PORT_COMPONENTS main)
  message(STATUS "MEMFAULT_PLATFORM_PORT_COMPONENTS not provided, using default ('${MEMFAULT_PLATFORM_PORT_COMPONENTS}')")
endif()

if(NOT DEFINED MEMFAULT_ESP_HTTP_CLIENT_ENABLE)
  if(DEFINED ENV{MEMFAULT_ESP_HTTP_CLIENT_ENABLE})
    set(MEMFAULT_ESP_HTTP_CLIENT_ENABLE $ENV{MEMFAULT_ESP_HTTP_CLIENT_ENABLE})
  else()
    set(MEMFAULT_ESP_HTTP_CLIENT_ENABLE 1)
  endif()
endif()

# Set variables used for version-specific checks later
set (MEMFAULT_IDF_VERSION_MINIMUM "4.4.0")
set(MEMFAULT_IDF_VERSION "${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}.${IDF_VERSION_PATCH}")
set(MEMFAULT_IDF_VERSION_ERROR_MSG "Memfault SDK requires ESP-IDF version ${MEMFAULT_IDF_VERSION_MINIMUM} or later. \
    Please contact mflt.io/contact-support for assistance.")

# Check that we are working with a supported version. If IDF_VERSION_MAJOR etc
# are unset (IDF versions before 4.0), this will catch that too.
if (MEMFAULT_IDF_VERSION VERSION_LESS MEMFAULT_IDF_VERSION_MINIMUM)
  message(FATAL_ERROR "${MEMFAULT_IDF_VERSION_ERROR_MSG}")
endif()

# Select RISCV or XTENSA architecture, depending on target chip
# idf_build_get_property was added in 4.0. RISC-V chips weren't added until
# v4.3, but we can rely on the build target for figuring out the architecture.
if(MEMFAULT_IDF_VERSION VERSION_GREATER_EQUAL 4)
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

list(APPEND MEMFAULT_COMPONENTS core util panics demo http metrics)
memfault_library(${MEMFAULT_SDK_ROOT} MEMFAULT_COMPONENTS
  MEMFAULT_COMPONENTS_SRCS MEMFAULT_COMPONENTS_INC_FOLDERS ${ESP_ARCH})

include($ENV{IDF_PATH}/tools/cmake/version.cmake OPTIONAL)

# Select version-specific porting files
if(MEMFAULT_IDF_VERSION VERSION_GREATER_EQUAL 6)
  set(MEMFAULT_ESP_IDF_PORT v6.x)
elseif(MEMFAULT_IDF_VERSION VERSION_GREATER_EQUAL 5)
  set(MEMFAULT_ESP_IDF_PORT v5.x)
elseif(MEMFAULT_IDF_VERSION VERSION_GREATER_EQUAL 4)
  set(MEMFAULT_ESP_IDF_PORT v4.x)
else()
  message(FATAL_ERROR "${MEMFAULT_IDF_VERSION_ERROR_MSG}")
endif()

# esp-idf version specific porting files
list(APPEND MEMFAULT_COMPONENTS_SRCS
  ${CMAKE_CURRENT_LIST_DIR}/${MEMFAULT_ESP_IDF_PORT}/memfault_esp_spi_flash.c
)

include(${CMAKE_CURRENT_LIST_DIR}/${MEMFAULT_ESP_IDF_PORT}/Memfault-esp-idf-compat.cmake)

# This directory holds ports that are common across v3.x and v4.x esp-idf releases
set(MEMFAULT_ESP_IDF_PORT_COMMON ${CMAKE_CURRENT_LIST_DIR}/common)
list(APPEND MEMFAULT_COMPONENTS_SRCS
  ${MEMFAULT_ESP_IDF_PORT_COMMON}/memfault_fault_handler.c
  ${MEMFAULT_ESP_IDF_PORT_COMMON}/memfault_platform_core.c
  ${MEMFAULT_ESP_IDF_PORT_COMMON}/memfault_platform_coredump.c
  ${MEMFAULT_ESP_IDF_PORT_COMMON}/memfault_platform_debug_log.c
  ${MEMFAULT_ESP_IDF_PORT_COMMON}/memfault_platform_device_info.c
  ${MEMFAULT_ESP_IDF_PORT_COMMON}/memfault_platform_http_client_buffer.c
  ${MEMFAULT_ESP_IDF_PORT_COMMON}/memfault_platform_http_client.c
  ${MEMFAULT_ESP_IDF_PORT_COMMON}/memfault_platform_metrics.c
)

if (CONFIG_MEMFAULT_CLI_ENABLED)
  list(APPEND MEMFAULT_COMPONENTS_SRCS
    ${MEMFAULT_ESP_IDF_PORT_COMMON}/memfault_platform_demo_cli_cmds.c
  )
endif()

if (CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD)
  list(APPEND MEMFAULT_COMPONENTS_SRCS
    ${MEMFAULT_ESP_IDF_PORT_COMMON}/memfault_platform_http_periodic_upload.c
  )
endif()

if (CONFIG_MEMFAULT_SYSTEM_TIME)
  list(APPEND MEMFAULT_COMPONENTS_SRCS
    ${MEMFAULT_ESP_IDF_PORT_COMMON}/memfault_platform_system_time.c
  )
endif()

if (CONFIG_MEMFAULT_DEEP_SLEEP_SUPPORT)
  list(APPEND MEMFAULT_COMPONENTS_SRCS
    ${MEMFAULT_ESP_IDF_PORT_COMMON}/memfault_platform_deep_sleep.c
  )
endif()

list(APPEND MEMFAULT_COMPONENTS_INC_FOLDERS
  include
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
  list(APPEND MEMFAULT_COMPONENTS_SRCS ${MEMFAULT_SDK_ROOT}/ports/freertos/src/memfault_sdk_metrics_thread.c)
  list(APPEND MEMFAULT_COMPONENTS_INC_FOLDERS ${MEMFAULT_SDK_ROOT}/ports/freertos/config)
endif()

if (CONFIG_MEMFAULT_MBEDTLS_METRICS)
  list(APPEND MEMFAULT_COMPONENTS_SRCS ${MEMFAULT_SDK_ROOT}/ports/mbedtls/memfault_mbedtls_metrics.c)
  list(APPEND MEMFAULT_COMPONENTS_INC_FOLDERS ${MEMFAULT_SDK_ROOT}/ports/mbedtls/config)
endif()

if (CONFIG_MEMFAULT_CLI_SELF_TEST)
  list(APPEND MEMFAULT_COMPONENTS_SRCS ${MEMFAULT_ESP_IDF_PORT_COMMON}/memfault_self_test_platform.c)
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
  ${MEMFAULT_PLATFORM_PORT_COMPONENTS}
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

if(${MEMFAULT_ESP_HTTP_CLIENT_ENABLE})
  list(APPEND COMPONENT_REQUIRES esp_http_client esp_https_ota)
endif()
register_component()

# sdk_overrides/esp_http_client.c needs the (private) esp_http header files:
get_filename_component(this_directory_name . ABSOLUTE DIRECTORY)
get_filename_component(this_directory_name ${this_directory_name} NAME)
mflt_esp32_component_get_target(this_component ${this_directory_name})

# Each esp-idf component is compiled into a static library. esp-idf then links each of these
# libraries to the program ELF. This can cause quirks when working with weak and strong symbol
# definitions within a static library causing the weak definition to be kept over the strong.
# To keep the strong definition around, we can define an undefined symbol to force the linker
# to reconsider different outputs. This allows the strong definition to be selected for inclusion
# in the program. When we're adding new strong definitions to weak functions internal to the SDK,
# the following is needed:
# 1. Add an empty stub function in the source file
# 2. Add a corresponding target_link_libraries call (see below) with the linker option -u
#    and specify the stub function name
if(CONFIG_MEMFAULT_CLI_SELF_TEST)
  target_link_libraries(${this_component} INTERFACE "-u memfault_esp_idf_include_self_test_impl")
endif()

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

component_compile_options(-DMEMFAULT_ESP_HTTP_CLIENT_ENABLE=${MEMFAULT_ESP_HTTP_CLIENT_ENABLE})

# FreeRTOS error logging redirect for ESP-IDF compatibility.
component_compile_options(
  -DMEMFAULT_FREERTOS_REGISTRY_FULL_ERROR_LOG_INCLUDE="memfault_platform_freertos_error_log.h"
)

# Extra compilation options set globally

if (MEMFAULT_IDF_VERSION VERSION_GREATER_EQUAL "5.0.0")
  # Empty for now, reserving for the future
else()
  # ESP-IDF < 5.0.0 forces the FreeRTOS config
  # INCLUDE_xTimerGetTimerDaemonTaskHandle=0, which is used to record timer task
  # stack in the SDK. Just disable the usage unconditionally.
  list(APPEND MEMFAULT_EXTRA_COMPILE_OPTIONS
    "-DMEMFAULT_FREERTOS_COLLECT_TIMER_STACK_FREE_BYTES=0"
  )
endif()


# Set the heartbeat config file to use the ESP-IDF port file, which will bring
# in the user's heartbeat config automatically. Set it as a global compiler
# option so it properly affects both component compilation and when the metric
# keys are used in the application.
#
# Set the ESP-IDF specific Memfault platform config header as well.
#
# Set these as compile_options, not compile_definitions; pre-v5 of ESP-IDF
# required the -D prefix, post-v5 auto strips it, but for safety, set them as
# plain options not "definitions"
list(APPEND compile_options
  "-DMEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE=\"memfault_esp_metrics_heartbeat_config.def\""
  "-DMEMFAULT_PLATFORM_CONFIG_FILE=\"memfault_esp_idf_port_config.h\""
  "-DMEMFAULT_TRACE_REASON_USER_DEFS_FILE=\"memfault_trace_reason_esp_idf_port_config.def\""
  ${MEMFAULT_EXTRA_COMPILE_OPTIONS}
)
idf_build_set_property(
  COMPILE_OPTIONS
  "${compile_options}"
  APPEND
)

# Compiling with -ggdb3 is only supported on ESP-IDF v5.5 and later, due to
# requiring this fix:
# https://github.com/espressif/esp-idf/commit/6d945bf0f6a6fc474bc2fee3ce09539172286b6d
if (MEMFAULT_IDF_VERSION VERSION_GREATER_EQUAL "5.5.0")
  if (CONFIG_MEMFAULT_GGDB3)
    # Enable -ggdb3 in compile_options.
    idf_build_set_property(
      COMPILE_OPTIONS
      "-ggdb3"
      APPEND
    )
  endif()
endif()

# We will intercept the panic handlers enabled by CONFIG_ESP32_ENABLE_COREDUMP_TO_FLASH
# and run the Memfault Fault Handler instead.
#
# Benefits here are:
#   FreeRTOS task list is walked server side instead of on device (so you can get crash data even if the lists are corrupted)
#   Much more flexibility in debug information collected (e.g. all RAM, just the current stack trace, select stacks and variables)
#   Data can be posted directly from device to Memfault cloud for deduplication and analysis
#
set(panic_handler_args "")
# In ESP-IDF v5.3.0, the core dump handler function was renamed from
# esp_core_dump_to_flash to esp_core_dump_write. It was backported to v5.2.2.
if (MEMFAULT_IDF_VERSION VERSION_GREATER_EQUAL "5.2.2")
  list(APPEND panic_handler_args "-Wl,--wrap=esp_core_dump_write")
# Backported to v5.1.4
elseif((MEMFAULT_IDF_VERSION VERSION_GREATER_EQUAL "5.1.4") AND (MEMFAULT_IDF_VERSION VERSION_LESS "5.2.0"))
  list(APPEND panic_handler_args "-Wl,--wrap=esp_core_dump_write")
else()
  list(APPEND panic_handler_args "-Wl,--wrap=esp_core_dump_to_flash")
endif()
list(
  APPEND panic_handler_args
  "-Wl,--wrap=esp_core_dump_init"
  "-Wl,--wrap=esp_core_dump_image_get"
)
target_link_libraries(${this_component} INTERFACE "${panic_handler_args}")

# Wrap panic_abort to intercept assert()/abort() calls. Note that support for
# this was added in ESP-IDF v4.2; earlier versions are intercepted with the
# __assert_func definition provided in memfault_stdlib_assert.c
target_link_libraries(${this_component} INTERFACE "-Wl,--wrap=panic_abort")

# Wrap allocator functions to capture mbedTLS metrics
if(CONFIG_MEMFAULT_MBEDTLS_METRICS)
  target_link_libraries(${this_component} INTERFACE "-Wl,--wrap=mbedtls_calloc -Wl,--wrap=mbedtls_free")
endif()

# Wrap to suppress errors when esp_event_create_loop_default() called multiple times
if (CONFIG_MEMFAULT_WRAP_EVENT_LOOP_CREATE_DEFAULT)
  target_link_libraries(${this_component} INTERFACE "-Wl,--wrap=esp_event_loop_create_default")
endif()

if(CONFIG_MEMFAULT_HEAP_STATS)
  target_link_libraries(${this_component} INTERFACE "-Wl,--wrap=malloc,--wrap=free")
  if (CONFIG_MEMFAULT_HEAP_STATS_CALLOC)
    target_link_libraries(${this_component} INTERFACE "-Wl,--wrap=calloc")
  endif()
endif()

if(CONFIG_MEMFAULT_METRICS_BOOT_TIME)
  target_link_libraries(${this_component} INTERFACE "-Wl,--wrap=esp_startup_start_app")
endif()

# Include a linker script fragment to support compact logs unconditionally. It's
# Include a linker script fragment to support compact logs unconditionally. It's
# only populated if compact logs are enabled, and otherwise is ignored. We keep
# it unconditionally included for backwards compatibility, so users updating the
# SDK don't need to change any build configuration.
get_filename_component(compact_log_linker_script ${MEMFAULT_ESP_IDF_PORT_COMMON}/memfault_compact_log.ld ABSOLUTE)
target_link_libraries(
  ${this_component}
  INTERFACE
  # Note: there is intentionally no space between -T and the linker script
  # file name. This is required for PlatformIO compatibility- the PlatformIO
  # build system inserts arguments into the linker command line, so we need to
  # make sure it can't separate the -T and the linker script file name.
  -T${compact_log_linker_script}
)

# Option to generate the Memfault Build ID, enabled by Kconfig. Only
# compatible with more recent ESP-IDF SDK versions, which have the 'app' target
# where the custom command is inserted.
if(CONFIG_MEMFAULT_USE_MEMFAULT_BUILD_ID)
  # We cannot use the 'EXECUTABLE'/'EXECUTABLE_NAME' build properties, because they are set
  # after component processing in the build process. Reconstruct it following the
  # same pattern, which works for typical idf.py builds.

  set(IDF_PROJECT_EXECUTABLE "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.elf")
  idf_build_get_property(python PYTHON)
  idf_build_get_property(elf_target EXECUTABLE GENERATOR_EXPRESSION)

  add_custom_command(OUTPUT "${IDF_PROJECT_EXECUTABLE}.memfault_log_fmt"
    # Compute and insert the build id
    COMMAND python ${MEMFAULT_SDK_ROOT}/scripts/fw_build_id.py ${IDF_PROJECT_EXECUTABLE}
    # Compress debug sections; this reduces the elf file size from ~10MB -> ~4.8MB
    COMMAND ${CMAKE_OBJCOPY} --compress-debug-sections ${IDF_PROJECT_EXECUTABLE}
    # Save a copy of the ELF that includes the 'log_fmt' section
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:$<GENEX_EVAL:${elf_target}>>" "$<TARGET_FILE:$<GENEX_EVAL:${elf_target}>>.memfault_log_fmt"
    COMMAND ${CMAKE_COMMAND} -E echo "*** NOTE: the symbol file to upload to app.memfault.com is $<TARGET_FILE:$<GENEX_EVAL:${elf_target}>>.memfault_log_fmt ***"
    # Remove the 'log_fmt' compact log section, which confuses elf2image
    COMMAND ${CMAKE_OBJCOPY} --remove-section log_fmt "$<TARGET_FILE:$<GENEX_EVAL:${elf_target}>>"
    # Update the timestamp of the .memfault_log_fmt file to be newer than its
    # just-edited dependency, ${IDF_PROJECT_EXECUTABLE}, so that the custom
    # target doesn't run every time
    COMMAND ${CMAKE_COMMAND} -E touch "$<TARGET_FILE:$<GENEX_EVAL:${elf_target}>>.memfault_log_fmt"
    DEPENDS "$<TARGET_FILE:$<GENEX_EVAL:${elf_target}>>"
  )

  if (MEMFAULT_IDF_VERSION VERSION_GREATER_EQUAL "5.6")
    add_custom_target(memfault_build_id DEPENDS "$<TARGET_FILE:$<GENEX_EVAL:${elf_target}>>.memfault_log_fmt")
    idf_build_add_post_elf_dependency("${CMAKE_PROJECT_NAME}.elf" memfault_build_id)
  else()
    # On ESP-IDF 5.5 and lower, we'll inject a dependency between
    # gen_project_binary and the ELF.

    # Dependency chain:
    # app -> gen_project_binary -> memfault_build_id -> ${IDF_PROJECT_EXECUTABLE}.memfault_log_fmt -> ${IDF_PROJECT_EXECUTABLE}
    add_custom_target(memfault_build_id DEPENDS "${IDF_PROJECT_EXECUTABLE}.memfault_log_fmt")
    if(CONFIG_SECURE_BOOT_BUILD_SIGNED_BINARIES)
      add_dependencies(gen_signed_project_binary memfault_build_id)
    else()
      add_dependencies(gen_project_binary memfault_build_id)
    endif()
  endif()  # ESP-IDF >= 5.6
endif()

# Link required libraries and add compiler flags needed for FreeRTOS region collection
# and LwIP metrics in >= 4.4.3.
# Note: CMake does not short-circuit logic statements, nested ifs required
if (MEMFAULT_IDF_VERSION VERSION_GREATER_EQUAL "4.4.3")
  # Policy change requires CMake v3.13+
  cmake_minimum_required(VERSION 3.13)

  # First set new policy for target_link_libraries, this resolves a warning when using on
  # targets not created in this directory
  cmake_policy(SET CMP0079 NEW)

  # Get the name of the ESP FreeRTOS target/library
  idf_component_get_property(freertos_lib freertos COMPONENT_LIB)

  # Link this component to FreeRTOS, use INTERFACE because we're only sharing headers
  target_link_libraries(${freertos_lib} INTERFACE ${this_component})

  target_compile_options(${freertos_lib} INTERFACE
    "-DMEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE=\"memfault_esp_metrics_heartbeat_config.def\""
    "-DMEMFAULT_PLATFORM_CONFIG_FILE=\"memfault_esp_idf_port_config.h\""
    "-DMEMFAULT_TRACE_REASON_USER_DEFS_FILE=\"memfault_trace_reason_esp_idf_port_config.def\""
  )

  # Lastly ensure that our FreeRTOS trace hooks are defined first by adding this
  # compile option to the FreeRTOS target to include with all source
  # This method is an alternative to #include within FreeRTOSConfig.h which esp-idf
  # makes very difficult to do.
  #
  # We exclude PlatformIO projects because PlatformIO's build system mechanism inserts other flags
  # in between the -include and the freertos_trace.h compile option. PlatformIO projects will need
  # to include the header independently in their project's .ini file. An example of how to do this
  # is at https://github.com/memfault/platformio-esp32-espidf
  if(NOT DEFINED PLATFORMIO_ENABLED)
    get_filename_component(freertos_trace_header ${MEMFAULT_SDK_ROOT}/ports/include/memfault/ports/freertos_trace.h ABSOLUTE)
    target_compile_options(${freertos_lib} INTERFACE
      -include ${freertos_trace_header}
    )
  endif()

  # Add definition for LWIP_STATS_LARGE to lwip component if using CONFIG_MEMFAULT_LWIP_METRICS
  if(CONFIG_MEMFAULT_LWIP_METRICS)
    # Get the name of the LwIP target/library
    idf_component_get_property(lwip_lib lwip COMPONENT_LIB)

    # Link this component to LwIP, use INTERFACE because we're only sharing headers
    target_link_libraries(${lwip_lib} INTERFACE ${this_component})

    target_compile_options(${lwip_lib} INTERFACE "-DLWIP_STATS_LARGE=1")
  endif()
endif()
