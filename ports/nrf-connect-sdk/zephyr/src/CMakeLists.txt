zephyr_library_sources(memfault_fault_handler.c)
zephyr_library_sources(memfault_platform_debug_log.c)
zephyr_library_sources_ifdef(CONFIG_MEMFAULT_SHELL memfault_demo_cli.c)
zephyr_library_sources_ifdef(CONFIG_MEMFAULT_HTTP_ENABLE memfault_platform_http.c)
zephyr_library_sources(memfault_platform_core.c)
zephyr_library_sources(memfault_platform_ram_backed_coredump.c)
zephyr_library_sources(memfault_zephyr_ram_regions.c)

# By default, crash data is saved in noinit RAM across a reboot (https://mflt.io/coredumps). A user
# can alternatively disable this and store crash state elsewhere (i.e internal flash, emmc, etc).
zephyr_library_sources_ifdef(CONFIG_MEMFAULT_RAM_BACKED_COREDUMP memfault_platform_ram_backed_coredump.c)

# Controls where root certificates are stored
zephyr_library_sources_ifdef(CONFIG_MEMFAULT_ROOT_CERT_STORAGE_NRF9160_MODEM memfault_nrf91_root_cert_storage.c)
zephyr_library_sources_ifdef(CONFIG_MEMFAULT_ROOT_CERT_STORAGE_TLS_CREDENTIAL_STORAGE memfault_tls_root_cert_storage.c)
