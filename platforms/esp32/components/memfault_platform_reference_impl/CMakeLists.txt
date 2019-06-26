# ESP-IDF SDK Overrides:
# NOTE: It's recommended to exclude the source files from the CMakeLists.txt
# files in the esp-idf that are being overridden here, to avoid accidentally
# linking in the original implementations.

set(MEMFAULT_SDK_OVERRIDES_SRCS
    sdk_overrides/esp_http_client.c
)

set(COMPONENT_SRCS
    memfault_esp32_platform.c
    memfault_platform_coredump_impl.c
    memfault_platform_debug_log.c
    memfault_platform_http_client.c
    ${MEMFAULT_SDK_OVERRIDES_SRCS}
)
set(COMPONENT_ADD_INCLUDEDIRS
    .
    sdk_overrides
)

set(COMPONENT_REQUIRES
    esp32
    esp_http_client
    freertos
    heap
    log
    memfault_core
    memfault_http
    memfault_panics
    soc
    spi_flash
)

register_component()

# sdk_overrides/esp_http_client.c needs the (private) esp_http header files:
mflt_esp32_component_get_target(this_component memfault_platform_reference_impl)
target_include_directories(${this_component} PRIVATE
    $ENV{IDF_PATH}/components/esp_http_client/include
    $ENV{IDF_PATH}/components/esp_http_client/lib/include
)
