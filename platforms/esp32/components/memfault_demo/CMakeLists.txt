set(COMPONENT_SRCS
    src/memfault_demo_cli.c
    src/memfault_demo_cli_aux.c
)
set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_REQUIRES
    memfault_core
    memfault_http
    memfault_panics
    memfault_platform_reference_impl
)

register_component()
