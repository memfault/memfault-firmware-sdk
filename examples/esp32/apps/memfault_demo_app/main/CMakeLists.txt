list(APPEND
   COMPONENT_SRCS
   cmd_app.c
   cmd_system.c
   console_example_main.c
   memfault_platform_device_info.c
)
if("${IDF_VERSION_MAJOR}" VERSION_GREATER_EQUAL 4)
   list(APPEND
      COMPONENT_SRCS
      cmd_wifi.c
   )
else()
   list(APPEND
      COMPONENT_SRCS
      cmd_wifi_legacy.c
   )
endif()

set(COMPONENT_ADD_INCLUDEDIRS .)

register_component()
