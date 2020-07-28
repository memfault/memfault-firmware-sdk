set(MEMFAULT_SDK_ROOT ../../..)

# Collect Memfault SDK dependencies
list(APPEND MEMFAULT_COMPONENTS core util panics demo)
include(${MEMFAULT_SDK_ROOT}/cmake/Memfault.cmake)
memfault_library(${MEMFAULT_SDK_ROOT} MEMFAULT_COMPONENTS
  MEMFAULT_COMPONENTS_SRCS MEMFAULT_COMPONENTS_INC_FOLDERS)

# Add Memfault SDK sources to memfault library
zephyr_interface_library_named(memfault)
zephyr_library()
zephyr_library_sources(${MEMFAULT_COMPONENTS_SRCS})
zephyr_include_directories(${MEMFAULT_COMPONENTS_INC_FOLDERS})

# Pick up Zephyr specific port files
add_subdirectory(src)

# By default, we save a stackdump, by saving crash state in RAM across a reboot
# (https://mflt.io/coredumps). A user can alternatively disable this and store crash state
# elsewhere (i.e internal flash, emmc, etc).
zephyr_library_sources_ifdef(CONFIG_MEMFAULT_RAM_BACKED_COREDUMP
  ${MEMFAULT_SDK_ROOT}/ports/panics/src/memfault_platform_ram_backed_coredump.c)

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

# Make sure RAM based storage regions are not initialized on boot
# by placing them in special linker sections
zephyr_linker_sources(NOINIT memfault-no-init.ld)

# The file user specified error traces are defined in:
#   https://mflt.io/error-tracing
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}
  -DMEMFAULT_TRACE_REASON_USER_DEFS_FILE=\\\"memfault_trace_reason_user_config.def\\\"")

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
