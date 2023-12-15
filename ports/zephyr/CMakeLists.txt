if(CONFIG_MEMFAULT)
  set(MEMFAULT_SDK_ROOT ../..)

  if(NOT DEFINED MEMFAULT_ZEPHYR_PORT_TARGET)
    # Pick up latest supported target by default.
    # Currently works for Zephyr >= 2.4
    set(MEMFAULT_ZEPHYR_PORT_TARGET v2.4)
  endif()

  # Collect Memfault SDK dependencies
  list(APPEND MEMFAULT_COMPONENTS core util panics demo)

  if(CONFIG_MEMFAULT_HTTP_ENABLE)
    list(APPEND MEMFAULT_COMPONENTS http)
  endif()

  if (CONFIG_MEMFAULT_METRICS)
    list(APPEND MEMFAULT_COMPONENTS metrics)
  endif()

  if(CONFIG_RISCV)
    set(MEMFAULT_ARCH "ARCH_RISCV")
  else()
    set(MEMFAULT_ARCH "ARCH_ARM_CORTEX_M")
  endif()

  include(${MEMFAULT_SDK_ROOT}/cmake/Memfault.cmake)
  memfault_library(${MEMFAULT_SDK_ROOT} MEMFAULT_COMPONENTS
    MEMFAULT_COMPONENTS_SRCS MEMFAULT_COMPONENTS_INC_FOLDERS ${MEMFAULT_ARCH})

  # Add Memfault SDK sources to memfault library
  zephyr_interface_library_named(memfault)
  zephyr_library()
  zephyr_library_sources(${MEMFAULT_COMPONENTS_SRCS})
  zephyr_include_directories(
    ${MEMFAULT_COMPONENTS_INC_FOLDERS}
    ${MEMFAULT_SDK_ROOT}/ports/include
    ${MEMFAULT_SDK_ROOT}/ports/zephyr/config
    include
  )

  # Be sure to use the Zephyr override config file if config'd in. This file
  # will include the base config file. zephyr_compile_definitions() use
  # INTERFACE as the scope so users will pickup this define as expected.
  zephyr_compile_definitions(MEMFAULT_PLATFORM_CONFIG_FILE=\"memfault_zephyr_platform_config.h\")

  # We automatically collect some Zephyr kernel metrics from a custom Zephyr port def file. The
  # Zephyr port def file will pull in the user's file via include directive.
  zephyr_compile_definitions(MEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE=\"memfault_metrics_heartbeat_zephyr_port_config.def\")

  # Zephyr specific port def for trace errors
  zephyr_compile_definitions(MEMFAULT_TRACE_REASON_USER_DEFS_FILE=\"memfault_trace_reason_zephyr_port_config.def\")

  # WORKAROUND: Add the macros defined in our compatibility header into library scope
  # This scope allows our library access to the macros but not applications or other zephyr modules
  # which is good for minimizing the workaround reach
  # Remove this line when removing support for < Zephyr v3.2
  zephyr_library_compile_options(-imacros ${CMAKE_CURRENT_LIST_DIR}/include/memfault/ports/zephyr/include_compatibility.h)

  # Pick up Zephyr specific port files
  add_subdirectory(${MEMFAULT_ZEPHYR_PORT_TARGET})
  add_subdirectory(common)
  add_subdirectory_ifdef(CONFIG_MEMFAULT_NRF_CONNECT_SDK ncs)

  # Link Memfault Library
  zephyr_library_link_libraries(memfault)
  target_link_libraries(memfault INTERFACE zephyr_interface)

  # The Zephyr includes moved from /include to /include/zephyr in Zephyr 3.2:
  # https://github.com/zephyrproject-rtos/zephyr/commit/53ef68d4598b2f9005c5da3fc0b860ca1999d350
  # Add the old path for backwards compatibility. Note that Zephyr itself
  # supports the Kconfig option 'CONFIG_LEGACY_INCLUDE_PATH' to enable the same
  # compatibility behavior, but it's marked as deprecated and to be removed in
  # the future, so just apply the compatibiltiy fix here.
  if(${KERNEL_VERSION_MAJOR}.${KERNEL_VERSION_MINOR}.${KERNEL_PATCHLEVEL} VERSION_GREATER_EQUAL "3.1.99" AND CONFIG_LEGACY_INCLUDE_PATH)
    zephyr_include_directories(${ZEPHYR_BASE}/include/zephyr)
  endif()

  # If enabled, apply a post-build step to generate a Memfault build id
  if(CONFIG_MEMFAULT_USE_MEMFAULT_BUILD_ID)
    get_filename_component(
      memfault_fw_build_id_script
      ${CMAKE_CURRENT_SOURCE_DIR}/../../scripts/fw_build_id.py
      ABSOLUTE
    )
    # force our command to be at the front of the list so it runs before any
    # other operations on the .elf file, including binary/OTA image generation
    get_property(
      PROPERTY_EXTRA_POST_BUILD_COMMANDS
      GLOBAL
      PROPERTY extra_post_build_commands
    )
    set_property(
      GLOBAL
      PROPERTY extra_post_build_commands
               COMMAND
               ${PYTHON_EXECUTABLE}
               ${memfault_fw_build_id_script}
               ${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.elf
               # now append the rest of the commands
               ${PROPERTY_EXTRA_POST_BUILD_COMMANDS}
    )
  endif()

endif()
