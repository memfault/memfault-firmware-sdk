# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# The default board for this example is the nrf52840dk_nrf52840, but can be
# overridden by passing --board=<board> to west or -DBOARD=<board> to cmake
set(BOARD nrf52840dk_nrf52840)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(fs_shell)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

zephyr_include_directories(config)

# Normalize dbug + __FILE__ paths to relative locations:
# 1. west workspace
# 2. toolchain install location (applies to paths in cross tool library files)
#
# If debugging from location 1., the west-manifest files should already have the
# correct relative path for GDB to locate them, otherwise the GDB "directory"
# command can be used to tell GDB where to find the source files (also required
# if the cross tool libraries/headers need to be located by GDB)
get_filename_component(WEST_PROJECT_ROOT_PATH
                       "${CMAKE_CURRENT_LIST_DIR}/.."
                       ABSOLUTE)
zephyr_compile_options(
    -ffile-prefix-map=${WEST_PROJECT_ROOT_PATH}=.
    -ffile-prefix-map=${TOOLCHAIN_HOME}=.
    -Wa,--debug-prefix-map=${WEST_PROJECT_ROOT_PATH}=.
)
