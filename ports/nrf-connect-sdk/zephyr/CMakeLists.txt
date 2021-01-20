if(CONFIG_MEMFAULT)

  # Latest version of nrf-connect-sdk targets v2.4 Zephyr so we use that as our default
  if(NOT DEFINED MEMFAULT_ZEPHYR_PORT_TARGET)
    set(MEMFAULT_ZEPHYR_PORT_TARGET v2.4)
  endif()

  set(MEMFAULT_SDK_ROOT ../../..)
  add_subdirectory(${MEMFAULT_SDK_ROOT}/ports/zephyr nrf-connect-sdk/zephyr)
  add_subdirectory(src)
  zephyr_include_directories(include)

endif()
