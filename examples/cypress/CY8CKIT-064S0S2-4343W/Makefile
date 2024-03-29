################################################################################
# \file Makefile
# \version 1.0
#
# \brief
# Top-level application make file.
#
################################################################################
# \copyright
# Copyright 2018-2019 Cypress Semiconductor Corporation
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################

# Choose between "aws_demos" and "aws_tests"
CY_AFR_BUILD=aws_demos

# Root location of AFR directory
CY_AFR_ROOT=../../../../../..

# Build artifact location
CY_BUILD_RELATIVE_LOCATION=$(CY_AFR_ROOT)/build/cy
CY_BUILD_LOCATION=$(abspath $(CY_BUILD_RELATIVE_LOCATION))

# Memfault
MEMFAULT_SDK_ROOT := ../../..
MEMFAULT_PORT_ROOT := $(MEMFAULT_SDK_ROOT)/examples/cypress/CY8CKIT-064S0S2-4343W/src

MEMFAULT_COMPONENTS := core util panics metrics
include $(MEMFAULT_SDK_ROOT)/makefiles/MemfaultWorker.mk

################################################################################
# Basic Configuration
################################################################################

# Target board/hardware
TARGET=CY8CKIT_064S0S2_4343W

# Name of application (used to derive name of final linked file).
APPNAME=$(CY_AFR_BUILD)

# Name of toolchain to use. Options include:
#
# GCC_ARM -- GCC 7.2.1, provided with ModusToolbox IDE
# ARM     -- ARM Compiler (must be installed separately)
# IAR     -- IAR Compiler (must be installed separately)
#
# See also: CY_COMPILER_PATH below
TOOLCHAIN=GCC_ARM

# Default build configuration. Options include:
#
# Debug   -- build with minimal optimizations, focus on debugging.
# Release -- build with full optimizations
CONFIG=Debug

# If set to "true" or "1", display full command-lines when building.
VERBOSE=


################################################################################
# Feature Configuration
################################################################################
# Enable or disable BLE module
BLE_SUPPORTED=1

# Set to 1 to add OTA defines, sources, and libraries (must be used with MCUBoot)
# NOTE: Extra code must be called from your app to initialize AFR OTA Agent
OTA_SUPPORT=0

# This platform always uses EXTERNAL_FLASH
OTA_USE_EXTERNAL_FLASH:=1

# Define CY_TEST_APP_VERSION_IN_TAR here to test application version
#        in TAR archive at start of OTA image download.
# NOTE: This requires that the version numbers here and in the header file match.
# NOTE: This will create compile warnings such as
#		'warning: "APP_VERSION_MAJOR" redefined'
#
# CY_TEST_APP_VERSION_IN_TAR=1
#
# APP_VERSION_MAJOR:=1
# APP_VERSION_MINOR:=0
# APP_VERSION_BUILD:=0

# CY_TFM_PSA_SUPPORTED feature cannot be disabled on this platform.
CY_TFM_PSA_SUPPORTED=1

# Using new Bootloader with SWAP / STATUS
CY_MCUBOOT_SWAP_USING_STATUS=1

################################################################################
# Advanced Configuration
################################################################################

# Enable optional code that is ordinarily disabled by default.
#
# Available components depend on the specific targeted hardware and firmware
# in use. In general, if you have
#
#    COMPONENTS=foo bar
#
# ... then code in directories named COMPONENT_foo and COMPONENT_bar will be
# added to the build
#
COMPONENTS=

# Like COMPONENTS, but disable optional code that was enabled by default.
DISABLE_COMPONENTS=

# By default the build system automatically looks in the Makefile's directory
# tree for source code and builds it. The SOURCES variable can be used to
# manually add source code to the build process from a location not searched
# by default, or otherwise not found by the build system.
SOURCES = \
	$(MEMFAULT_COMPONENTS_SRCS) \
	$(MEMFAULT_SDK_ROOT)/ports/freertos/src/memfault_metrics_freertos.c \
	$(MEMFAULT_SDK_ROOT)/ports/freertos/src/memfault_core_freertos.c \
	$(MEMFAULT_SDK_ROOT)/ports/freertos/src/memfault_freertos_ram_regions.c

# Like SOURCES, but for include directories. Value should be paths to
# directories (without a leading -I).
# TODO remove this once whd is update to new version
INCLUDES = \
	$(CY_AFR_ROOT)/vendors/cypress/MTB/libraries/wifi-host-driver/WiFi_Host_Driver/resources/nvram/TARGET_CY8CKIT_064B0S2_4343W \
	$(MEMFAULT_COMPONENTS_INC_FOLDERS) \
	$(MEMFAULT_SDK_ROOT)/ports/include \
	$(MEMFAULT_PORT_ROOT)

# Add additional defines to the build process (without a leading -D).
DEFINES=

# Select softfp or hardfp floating point. Default is softfp.
VFP_SELECT=

# Additional / custom C compiler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
CFLAGS=

# Additional / custom C++ compiler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
CXXFLAGS=

# Additional / custom assembler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
ASFLAGS=

# Additional / custom linker flags.
LDFLAGS=-Wl,--build-id

# Additional / custom libraries to link in to the application.
LDLIBS=

# Path to the linker script to use (if empty, use the default linker script).
export LINKER_SCRIPT=cyb06xxa_cm4_dual.ld

# Custom pre-build commands to run.
PREBUILD=

# Custom post-build commands to run.
POSTBUILD=


################################################################################
# Paths
################################################################################

# Relative path to the project directory (default is the Makefile's directory).
#
# This controls where automatic source code discovery looks for code.
CY_APP_PATH=

# Relative path to the "base" library. It provides the core makefile build
# infrastructure.
CY_BASELIB_PATH=$(CY_AFR_ROOT)/vendors/cypress/MTB/psoc6/psoc6make

# Absolute path to the compiler's "bin" directory.
#
# The default depends on the selected TOOLCHAIN (GCC_ARM uses the ModusToolbox
# IDE provided compiler by default).
CY_COMPILER_PATH=

# Include aFR configuration make file
include $(CY_AFR_ROOT)/projects/cypress/make_support/afr.mk


################################################################################
# Tools path
################################################################################

# Locate ModusToolbox IDE helper tools folders in default installation
# locations for Windows, Linux, and macOS.
CY_WIN_HOME=$(subst \,/,$(USERPROFILE))
CY_TOOLS_PATHS ?= $(wildcard \
    $(CY_WIN_HOME)/ModusToolbox/tools_* \
    $(HOME)/ModusToolbox/tools_* \
    /Applications/ModusToolbox/tools_*)

# If you install ModusToolbox IDE in a custom location, add the path to its
# "tools_X.Y" folder (where X and Y are the version number of the tools
# folder).
CY_TOOLS_PATHS+=

# Default to the newest installed tools folder, or the users override (if it's
# found).
CY_TOOLS_DIR=$(lastword $(sort $(wildcard $(CY_TOOLS_PATHS))))

ifeq ($(CY_TOOLS_DIR),)
$(error Unable to find any of the available CY_TOOLS_PATHS -- $(CY_TOOLS_PATHS))
endif

$(info Tools Directory: $(CY_TOOLS_DIR))
include $(CY_TOOLS_DIR)/make/start.mk
