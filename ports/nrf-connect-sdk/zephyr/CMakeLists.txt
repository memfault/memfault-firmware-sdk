if(CONFIG_MEMFAULT)

  # Latest version of nrf-connect-sdk targets v2.4 Zephyr so we use that as our default
  if(NOT DEFINED MEMFAULT_ZEPHYR_PORT_TARGET)
    set(MEMFAULT_ZEPHYR_PORT_TARGET v2.4)
  endif()

  if (${KERNEL_VERSION_MAJOR} EQUAL 2)
    if (${KERNEL_VERSION_MINOR} LESS_EQUAL 3)
      if (CONFIG_MEMFAULT_HTTP_MAX_POST_SIZE EQUAL 0)
        message(FATAL_ERROR "CONFIG_MEMFAULT_HTTP_MAX_POST_SIZE=256 must be added to your prj.conf"
          " when using an nRF Connect SDK version older than v1.4")
      endif()
    endif()
  endif()

  set(MEMFAULT_SDK_ROOT ../../..)
  add_subdirectory(${MEMFAULT_SDK_ROOT}/ports/zephyr nrf-connect-sdk/zephyr)
  add_subdirectory(src)
  zephyr_include_directories(include)

endif()
