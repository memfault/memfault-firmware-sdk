NAME := MemfaultTestApp

GLOBAL_DEFINES     += DEBUG

THIS_DIR := $(dir $(word $(words $(MAKEFILE_LIST)), $(MAKEFILE_LIST)))

MEMFAULT_SDK_ROOT := ../../../..

GLOBAL_INCLUDES += \
  $(MEMFAULT_SDK_ROOT)/components/include \
  config


$(NAME)_SOURCES := memfault_demo_app.c \


$(NAME)_COMPONENTS := \
  utilities/command_console \
  utilities/command_console/wifi \
  utilities/wiced_log \
  libraries/memfault/core \
  libraries/memfault/demo \
  libraries/memfault/http \
  libraries/memfault/panics \
  libraries/memfault/util \
  libraries/memfault/platform_reference_impl \


VALID_OSNS_COMBOS  := ThreadX-NetX_Duo FreeRTOS-LwIP

VALID_PLATFORMS := \
  BCM943362WCD4 \
  BCM943362WCD6 \
  BCM943362WCD8 \
  BCM943364WCD1 \
  CYW94343WWCD1_EVB \
  BCM943438WCD1 \
  BCM94343WWCD2 \
  CY8CKIT_062 \
  NEB1DX* \
  CYW9MCU7X9N364 \
  CYW943907AEVAL1F \
  CYW954907AEVAL1F \
  CYW9WCD2REFAD2* \
  CYW9WCD760PINSDAD2 \
  CYW943455EVB* \
  CYW943012EVB*
