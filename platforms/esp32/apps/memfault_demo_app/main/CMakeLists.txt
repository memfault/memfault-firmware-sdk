set(COMPONENT_SRCS "cmd_memfault.c"
                   "cmd_system.c"
                   "cmd_wifi.c"
                   "config.c"
                   "console_example_main.c")
set(COMPONENT_ADD_INCLUDEDIRS ".")

register_component()

set(COMPONENT_REQUIRES
    memfault_core
    memfault/demo
    memfault/http
    memfault/panics
    memfault/platform_reference_impl
)
