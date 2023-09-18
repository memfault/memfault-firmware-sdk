list(APPEND
   COMPONENT_SRCS
   cmd_app.c
   cmd_system.c
   console_example_main.c
   led.c
   memfault/memfault_platform_device_info.c
)

if (CONFIG_APP_MEMFAULT_TRANSPORT_HTTP)
   list(APPEND COMPONENT_SRCS app_memfault_transport_http.c)
elseif(CONFIG_APP_MEMFAULT_TRANSPORT_MQTT)
   list(APPEND COMPONENT_SRCS app_memfault_transport_mqtt.c)
endif()

# the 'cmd_wifi.c' implementation is different for ESP-IDF v5+
if("${IDF_VERSION_MAJOR}" VERSION_GREATER_EQUAL 5)
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

set(COMPONENT_ADD_INCLUDEDIRS
   .
   memfault
)

register_component()
