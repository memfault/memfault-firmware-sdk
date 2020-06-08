# A convenience helper cmake function that can be used to collect the sources and include
# paths needed for the Memfault SDK based on the components used
#
# USAGE
# If you are using a Cmake build system, to pick up the Memfault include paths & source
# files needed for a project, you can just add the following lines:
#
# set(MEMFAULT_SDK_ROOT <The path to the root of the memfault-firmware-sdk repo>)
# list(APPEND MEMFAULT_COMPONENTS <The SDK components to be used, i.e "core util">)
# include(${MEMFAULT_SDK_ROOT}/cmake/MemfaultWorker.cmake)
# memfault_library(${MEMFAULT_SDK_ROOT} MEMFAULT_COMPONENTS
#   MEMFAULT_COMPONENTS_SRCS MEMFAULT_COMPONENTS_INC_FOLDERS)
#
# NOTE: By default, the list of sources returned will be for ARM Cortex-M targets but ARCH_XTENSA can be
# passed as an optional final argument to return sources expected for ARCH_XTENSA architectures
#
# After invoking the function ${MEMFAULT_COMPONENTS_SRCS} will contain the sources
# needed for the library and ${MEMFAULT_COMPONENTS_INC_FOLDERS} will contain the include
# paths

function(memfault_library sdk_root components src_var_name inc_var_name)
  if(NOT IS_ABSOLUTE ${sdk_root})
    set(sdk_root "${CMAKE_CURRENT_SOURCE_DIR}/${sdk_root}")
  endif()

  foreach(component IN LISTS ${components})
    file(GLOB MEMFAULT_COMPONENT_${component} ${sdk_root}/components/${component}/src/*.c)
    list(APPEND SDK_SRC ${MEMFAULT_COMPONENT_${component}})
    list(APPEND SDK_INC ${sdk_root}/components/${component}/include)
  endforeach()
  set(arch ${ARGN})
  if(NOT arch)
    set(arch ARCH_ARM_CORTEX_M)
  endif()

  if(arch STREQUAL "ARCH_XTENSA")
    list(REMOVE_ITEM SDK_SRC ${sdk_root}/components/panics/src/memfault_fault_handling_arm.c)
    list(REMOVE_ITEM SDK_SRC ${sdk_root}/components/panics/src/arch_arm_cortex_m.c)
  elseif(arch STREQUAL "ARCH_ARM_CORTEX_M")
    list(REMOVE_ITEM SDK_SRC ${sdk_root}/components/panics/src/memfault_fault_handling_xtensa.c)
  else()
    message(FATAL_ERROR "Unsupported Arch: ${arch}")
  endif()
  set(${src_var_name} ${SDK_SRC} PARENT_SCOPE)
  set(${inc_var_name} ${SDK_INC} PARENT_SCOPE)
endfunction()
