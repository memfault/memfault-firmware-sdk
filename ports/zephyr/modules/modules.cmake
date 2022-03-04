if(PROJECT_VERSION VERSION_LESS 2.6.0)
  set(ZEPHYR_MEMFAULT_FIRMWARE_SDK_KCONFIG ${CMAKE_CURRENT_LIST_DIR}/../Kconfig.2_5_0)
else()
  # When a newer Zephyr with incompatible Kconfig tree is released then update
  # the `else()` to `elseif(PROJECT_VERSION VERSION_LESS <incompatible-version>)`
  # and then add a new Kconfig.<version> file to accomodate changes in the new Zephyr.
  set(ZEPHYR_MEMFAULT_FIRMWARE_SDK_KCONFIG ${CMAKE_CURRENT_LIST_DIR}/../Kconfig.2_6_0)
endif()
