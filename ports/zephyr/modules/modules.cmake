# This file is used by Zephyr when processing the Memfault module, via the
# "Module integration files (external)" feature:
# https://docs.zephyrproject.org/latest/develop/modules.html#module-integration-files-external
#
# Memfault uses this to selectively enable certain Kconfig features and options
# based on Zephyr version. This is necessary because Zephyr's Kconfig tree
# changes over time, and Memfault needs to support multiple versions of Zephyr.

if(PROJECT_VERSION VERSION_LESS 3.7.0)
  set(ZEPHYR_MEMFAULT_FIRMWARE_SDK_KCONFIG ${CMAKE_CURRENT_LIST_DIR}/../Kconfig)
else()
  # When a newer Zephyr with incompatible Kconfig tree is released then update
  # the `else()` to `elseif(PROJECT_VERSION VERSION_LESS <incompatible-version>)`
  # and then add a new Kconfig.<version> file to accommodate changes in the new Zephyr.
  set(ZEPHYR_MEMFAULT_FIRMWARE_SDK_KCONFIG ${CMAKE_CURRENT_LIST_DIR}/../Kconfig.3_7_0)
endif()
