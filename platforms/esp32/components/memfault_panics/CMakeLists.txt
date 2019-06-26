set(COMPONENT_SRCS
    src/memfault_coredump.c
    src/memfault_fault_handling_xtensa.c
    src/memfault_ram_reboot_info_tracking.c
)
set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_REQUIRES
    memfault_core
)

register_component()
