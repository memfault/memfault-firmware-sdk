zephyr_library_sources(memfault_fault_handler.c)
zephyr_include_directories(.)

# Make sure RAM based storage regions are not initialized on boot
# by placing them in special linker sections
zephyr_linker_sources(NOINIT memfault-no-init.ld)


