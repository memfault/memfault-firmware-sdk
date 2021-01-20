# Make sure RAM based storage regions are not initialized on boot
# by placing them in special linker sections
zephyr_linker_sources(NOINIT memfault-no-init.ld)

zephyr_linker_sources(SECTIONS memfault-build-id.ld)

# Override the default Zephyr setting which disables the GNU Build ID
#   https://github.com/zephyrproject-rtos/zephyr/blob/d7ee114106eab485688223d97a49813d33b4cf21/cmake/linker/ld/target_base.cmake#L16
zephyr_ld_options("-Wl,--build-id")

# Add a unique identier to the firmware build
#
# It is very common, especially during development, to not change the firmware
# version between editing and compiling the code. This will lead to issues when
# recovering backtraces or symbol information because the debug information in
# the symbol file may be out of sync with the actual binary. Tracking a build id
# enables the Memfault cloud to identify and surface when this happens!
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DMEMFAULT_USE_GNU_BUILD_ID=1" PARENT_SCOPE)
