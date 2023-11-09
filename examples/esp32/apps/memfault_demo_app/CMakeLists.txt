# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(PROJECT_NAME memfault-esp32-demo-app)
set(CMAKE_PROJECT_NAME ${PROJECT_NAME})

include($ENV{IDF_PATH}/tools/cmake/version.cmake OPTIONAL)

# This contains the implementation for the led_strip driver. In ESP-IDF v5 it
# moved to a "managed component" and no longer needs to be manually added.
if(DEFINED IDF_VERSION_MAJOR)
  if(IDF_VERSION_MAJOR VERSION_LESS 5)
    set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/led_strip)
  endif()
endif()

# Look for the Memfault SDK in a subdirectory first, when this app is used
# standalone (not from within the Memfault SDK)
get_filename_component(memfault_firmare_sdk_dir third-party/memfault-firmware-sdk ABSOLUTE)
if(NOT EXISTS ${memfault_firmare_sdk_dir})
get_filename_component(memfault_firmare_sdk_dir ../../../../ ABSOLUTE)
endif()
include(${memfault_firmare_sdk_dir}/ports/esp_idf/memfault.cmake)

# NOTE: This include also applies global compiler options, make sure
# this happens first before defining other targets!
# The esp-idf project() macro creates a project_name.elf target:
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(${PROJECT_NAME})

# Check for invalid partition table configurations
if (
  CONFIG_APP_MEMFAULT_TRANSPORT_HTTP AND
  NOT CONFIG_PARTITION_TABLE_CUSTOM_FILENAME STREQUAL "partitions_example.csv"
)
  message(WARNING "Data transport is HTTP but using partition table ${CONFIG_PARTITION_TABLE_CUSTOM_FILENAME}")
  set(INVALID_PARTITION_TABLE true)
elseif(
  CONFIG_APP_MEMFAULT_TRANSPORT_MQTT AND
  NOT CONFIG_PARTITION_TABLE_CUSTOM_FILENAME STREQUAL "partitions_example_mqtt.csv"
)
  message(WARNING "Data transport is MQTT but using partition table ${CONFIG_PARTITION_TABLE_CUSTOM_FILENAME}")
  set(INVALID_PARTITION_TABLE true)
endif()

if (INVALID_PARTITION_TABLE)
  message(FATAL_ERROR
    "Invalid partition table configuration, check CONFIG_APP_MEMFAULT_TRANSPORT configs.\
    If this error occurs repeatedly run `idf.py fullclean && rm sdkconfig`"
  )
endif()

# Add the Memfault Build ID so each build can have a unique version.
set(IDF_PROJECT_EXECUTABLE ${PROJECT_NAME}.elf)
add_custom_command(TARGET ${IDF_PROJECT_EXECUTABLE}
  POST_BUILD
  # Compute and insert the build id
  COMMAND python ${memfault_firmare_sdk_dir}/scripts/fw_build_id.py ${IDF_PROJECT_EXECUTABLE}
  # Save a copy of the ELF that includes the 'log_fmt' section
  BYPRODUCTS ${IDF_PROJECT_EXECUTABLE}.memfault_log_fmt
  # Compress debug sections; this reduces the elf file size from ~10MB -> ~4.8MB
  COMMAND ${CMAKE_OBJCOPY} --compress-debug-sections ${IDF_PROJECT_EXECUTABLE}
  COMMAND ${CMAKE_COMMAND} -E copy ${IDF_PROJECT_EXECUTABLE} ${IDF_PROJECT_EXECUTABLE}.memfault_log_fmt
  COMMAND ${CMAKE_COMMAND} -E echo "*** NOTE: the symbol file to upload to app.memfault.com is ${IDF_PROJECT_EXECUTABLE}.memfault_log_fmt ***"
  # Remove the 'log_fmt' compact log section, which confuses elf2image
  COMMAND ${CMAKE_OBJCOPY} --remove-section log_fmt ${IDF_PROJECT_EXECUTABLE}
)
