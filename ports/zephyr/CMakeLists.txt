if(CONFIG_MEMFAULT)
  set(MEMFAULT_SDK_ROOT ../..)

  # Collect Memfault SDK dependencies
  list(APPEND MEMFAULT_COMPONENTS core util panics demo)

  if(CONFIG_MEMFAULT_HTTP_ENABLE)
    list(APPEND MEMFAULT_COMPONENTS http)
  endif()

  if (CONFIG_MEMFAULT_METRICS)
    list(APPEND MEMFAULT_COMPONENTS metrics)
  endif()



  include(${MEMFAULT_SDK_ROOT}/cmake/Memfault.cmake)
  memfault_library(${MEMFAULT_SDK_ROOT} MEMFAULT_COMPONENTS
    MEMFAULT_COMPONENTS_SRCS MEMFAULT_COMPONENTS_INC_FOLDERS)

  # Add Memfault SDK sources to memfault library
  zephyr_interface_library_named(memfault)
  zephyr_library()
  zephyr_library_sources(${MEMFAULT_COMPONENTS_SRCS})
  zephyr_include_directories(
    ${MEMFAULT_COMPONENTS_INC_FOLDERS}
    ${MEMFAULT_SDK_ROOT}/ports/include
    include
  )

  if(${MEMFAULT_ZEPHYR_PORT_TARGET} MATCHES "^v2\.*")
    add_subdirectory(v2.x)
  endif()

  # Pick up Zephyr specific port files
  add_subdirectory(${MEMFAULT_ZEPHYR_PORT_TARGET})
  add_subdirectory(common)

  # Link Memfault Library
  zephyr_library_link_libraries(memfault)
  target_link_libraries(memfault INTERFACE zephyr_interface)

endif()
