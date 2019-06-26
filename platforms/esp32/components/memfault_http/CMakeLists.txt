set(COMPONENT_SRCS
    src/memfault_http_client.c
)
set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_REQUIRES
    memfault_core
    memfault_panics
)

register_component()
