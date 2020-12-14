if(CONFIG_MEMFAULT)

  set(MEMFAULT_SDK_ROOT ../../..)

  # Collect Memfault SDK dependencies
  list(APPEND MEMFAULT_COMPONENTS core util panics demo)

  if(CONFIG_MEMFAULT_HTTP_ENABLE)
    list(APPEND MEMFAULT_COMPONENTS http)
  endif()

  include(${MEMFAULT_SDK_ROOT}/cmake/Memfault.cmake)
  memfault_library(${MEMFAULT_SDK_ROOT} MEMFAULT_COMPONENTS
    MEMFAULT_COMPONENTS_SRCS MEMFAULT_COMPONENTS_INC_FOLDERS)

  # Add Memfault SDK sources to memfault library
  zephyr_interface_library_named(memfault)
  zephyr_library()
  zephyr_library_sources(${MEMFAULT_COMPONENTS_SRCS})
  zephyr_include_directories(${MEMFAULT_COMPONENTS_INC_FOLDERS} include)

  # Pick up Zephyr specific port files
  add_subdirectory(src)

  # Zephyr fatals for ARM Cortex-M's take the following path:
  #
  # frame 3 k_sys_fatal_error_handler    nrf/lib/fatal_error/fatal_error.c
  #                                      (overrides weak impl in zephyr/kernel/fatal.c)
  #  frame 2 z_fatal_error               zephyr/kernel/fatal.c
  #   frame 1 z_arm_fault                zephyr/arch/arm/core/aarch32/cortex_m/fault.c
  #    frame 0 z_arm_hard_fault          zephyr/arch/arm/core/aarch32/cortex_m/fault_s.S
  #
  # The nrf-connect-sdk does not expose a mechanism to override "k_sys_fatal_error_handler"
  # and simply reboots the system so instead we intercept the frame above and install the
  # Memfault fault handler which will collect a coredump.
  target_link_libraries(app INTERFACE "-Wl,--wrap=z_fatal_error")

  # We trace task creation so the task TCBs and stacks can be collected at the time
  # of a crash and all thread backtraces can be made available in the Memfault UI.
  #
  # To do this we wrap the arch_new_thread() function call that is made from thread.c
  #
  # A nicer way to do this would be to make use of the "sys_trace_thread_create" macro.
  # Unfortunately, to override the macro, one must patch the Zephyr RTOS today.
  #   https://github.com/zephyrproject-rtos/zephyr/blob/390537b/include/tracing/tracing.h#L57-L61
  target_link_libraries(app INTERFACE "-Wl,--wrap=arch_new_thread")


  # Make sure RAM based storage regions are not initialized on boot
  # by placing them in special linker sections
  zephyr_linker_sources(NOINIT memfault-no-init.ld)

  # Add a unique identier to the firmware build
  #
  # It is very common, especially during development, to not change the firmware
  # version between editing and compiling the code. This will lead to issues when
  # recovering backtraces or symbol information because the debug information in
  # the symbol file may be out of sync with the actual binary. Tracking a build id
  # enables the Memfault cloud to identify and surface when this happens!
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DMEMFAULT_USE_GNU_BUILD_ID=1")
  zephyr_linker_sources(SECTIONS memfault-build-id.ld)
  # Override the default Zephyr setting which disables the GNU Build ID
  #   https://github.com/zephyrproject-rtos/zephyr/blob/d7ee114106eab485688223d97a49813d33b4cf21/cmake/linker/ld/target_base.cmake#L16
  zephyr_ld_options("-Wl,--build-id")

  # Link Memfault Library
  zephyr_library_link_libraries(memfault)
  target_link_libraries(memfault INTERFACE zephyr_interface)
endif()
