if(CONFIG_MEMFAULT_NRF_CONNECT_SDK)

  if (${KERNEL_VERSION_MAJOR} EQUAL 2)
    if (${KERNEL_VERSION_MINOR} LESS_EQUAL 3)
      if (CONFIG_MEMFAULT_HTTP_MAX_POST_SIZE EQUAL 0)
        message(FATAL_ERROR "CONFIG_MEMFAULT_HTTP_MAX_POST_SIZE=256 must be added to your prj.conf"
          " when using an nRF Connect SDK version older than v1.4")
      endif()
    endif()
  endif()

  add_subdirectory(src)
  zephyr_include_directories(include)

  # Note: Starting in nRF Connect SDK >= 1.3, NCS_VERSION_* fields are exposed.
  # Prior to that, they're unset and cause the VERSION_GREATER_EQUAL check to be
  # falsy
  if(CONFIG_MEMFAULT_FOTA
     AND (${NCS_VERSION_MAJOR}.${NCS_VERSION_MINOR}.${NCS_VERSION_PATCH}
          VERSION_GREATER_EQUAL
          2.4.0)
     # nRF Connect SDK 2.6.0+ natively supports multiple root certificates, so
     # this workaround is no longer needed. Note that 2.5.99 is the development
     # version for 2.6.0, so use that as the upper bound to support pre-release
     # versions.
     AND (${NCS_VERSION_MAJOR}.${NCS_VERSION_MINOR}.${NCS_VERSION_PATCH}
          VERSION_LESS
          2.5.99)
  )
    # We wrap download_client_get() in order to register multiple root certificates
    #   See comment in src/memfault_fota.c for more details
    zephyr_ld_options(-Wl,--wrap=download_client_get)
  endif()

endif()
