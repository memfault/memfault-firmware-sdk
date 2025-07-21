# Zephyr Example Application

cmake_minimum_required(VERSION 3.20.0)

# The default board for this example is the mps2/an385, but can be
# overridden by passing --board=<board> to west or -DBOARD=<board> to cmake
set(BOARD mps2/an385)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

target_sources(app PRIVATE
  src/cdr.c
  src/main.c
  src/metrics.c
)
zephyr_include_directories(config)
project(qemu-app LANGUAGES C VERSION 1.0.0)

# Generate a git hash that's used as part of the software_version, eg
# 1.0.0+12345678. Permit overriding at command line for CI builds.
if (NOT DEFINED ZEPHYR_MEMFAULT_EXAMPLE_GIT_SHA1)
  EXECUTE_PROCESS(
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND git rev-parse --short HEAD
    RESULT_VARIABLE commit_sha1
    OUTPUT_VARIABLE ZEPHYR_MEMFAULT_EXAMPLE_GIT_SHA1
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY
  )
endif()

zephyr_compile_definitions(
  ZEPHYR_MEMFAULT_EXAMPLE_GIT_SHA1=\"${ZEPHYR_MEMFAULT_EXAMPLE_GIT_SHA1}\"
)

zephyr_compile_options(--param=min-pagesize=0x1000)

# For Zephyr >=3.0.0,<3.6.0, explicitly enable -Werror
if (${KERNEL_VERSION_MAJOR}.${KERNEL_VERSION_MINOR}.${KERNEL_PATCHLEVEL} VERSION_GREATER_EQUAL 3.0.0 AND
    ${KERNEL_VERSION_MAJOR}.${KERNEL_VERSION_MINOR}.${KERNEL_PATCHLEVEL} VERSION_LESS 3.6.0)
  zephyr_compile_options(-Werror)
endif()
