# Controls where root certificates are stored
zephyr_library_sources_ifdef(CONFIG_MEMFAULT_ROOT_CERT_STORAGE_NRF9160_MODEM memfault_nrf91_root_cert_storage.c)
zephyr_library_sources_ifdef(CONFIG_MEMFAULT_REBOOT_REASON_GET_CUSTOM nrfx_pmu_reboot_tracking.c)
zephyr_library_sources_ifdef(CONFIG_MEMFAULT_NRF_PLATFORM_BATTERY_NPM13XX memfault_platform_npm13xx_battery.c)

if (${NCS_VERSION_MAJOR}.${NCS_VERSION_MINOR}.${NCS_VERSION_PATCH} VERSION_GREATER_EQUAL 2.9.99)
  zephyr_library_sources_ifdef(CONFIG_MEMFAULT_FOTA memfault_fota.c)
else()
  zephyr_library_sources_ifdef(CONFIG_MEMFAULT_FOTA memfault_fota_legacy.c)
endif()

zephyr_library_sources_ifdef(CONFIG_MEMFAULT_NRF_SHELL memfault_nrf_cli.c)
zephyr_library_sources_ifdef(CONFIG_MEMFAULT_NRF_CONNECTIVITY_CONNECTED_TIME_NRF91X memfault_platform_metrics_connectivity_lte.c)

if (CONFIG_MEMFAULT_USE_NRF_CLOUD_TRANSPORT)
  # if Zephyr is less than 3.5.0, print an error
  if(${KERNEL_VERSION_MAJOR}.${KERNEL_VERSION_MINOR}.${KERNEL_VERSION_PATCH} VERSION_LESS "3.5.0")
    message(FATAL_ERROR
      "Memfault nRF Cloud CoAP transport requires Zephyr version 3.5.0 or newer."
    )
  endif()

  zephyr_library_sources(memfault_platform_coap.c)
endif()
