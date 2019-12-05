set(MEMFAULT_SDK_ROOT ../../)

# Collect Memfault SDK dependencies
list(APPEND MEMFAULT_COMPONENTS core util panics demo http)
include(${MEMFAULT_SDK_ROOT}/cmake/Memfault.cmake)
memfault_library(${MEMFAULT_SDK_ROOT} MEMFAULT_COMPONENTS
  MEMFAULT_COMPONENTS_SRCS MEMFAULT_COMPONENTS_INC_FOLDERS)

# Add Memfault SDK sources to memfault library
zephyr_interface_library_named(memfault)
zephyr_library()
zephyr_library_sources(${MEMFAULT_COMPONENTS_SRCS})
zephyr_include_directories(${MEMFAULT_COMPONENTS_INC_FOLDERS})

# Pick up Zephyr specific port files
add_subdirectory(${MEMFAULT_ZEPHYR_PORT_TARGET})
zephyr_library_sources(${MEMFAULT_SDK_ROOT}/ports/panics/src/memfault_platform_ram_backed_coredump.c)
add_subdirectory(common)

# Link Memfault Library
zephyr_library_link_libraries(memfault)
target_link_libraries(memfault INTERFACE zephyr_interface)
