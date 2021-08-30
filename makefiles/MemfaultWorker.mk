# A convenience helper makefile that can be used to collect the sources and include
# flags needed for the Memfault SDK based on the components used
#
# USAGE
# If you are using a Make build system, to pick up the Memfault include paths & source
# files needed for a project, you can just add the following lines:
#
# MEMFAULT_SDK_ROOT := <The path to the root of the memfault-firmware-sdk repo>
# MEMFAULT_COMPONENTS := <The SDK components to be used, i.e "core panics util">
# include $(MEMFAULT_SDK_ROOT)/makefiles/MemfaultWorker.mk
# <YOUR_SRC_FILES> += $(MEMFAULT_COMPONENTS_SRCS)
# <YOUR_INCLUDE_PATHS> += $(MEMFAULT_COMPONENTS_INC_FOLDERS)

# A utility to easily assert that a Makefile variable is defined and non-empty
#   Argument 1: The variable to check
#   Argument 2: The error message to display if the variable is not defined
memfault_assert_arg_defined = \
  $(if $(value $(strip $1)),,$(error Undefined $1:$2))

MEMFAULT_VALID_COMPONENTS := core demo http panics util metrics

$(call memfault_assert_arg_defined,MEMFAULT_COMPONENTS,\
  Must be set to one or more of "$(MEMFAULT_VALID_COMPONENTS)")
$(call memfault_assert_arg_defined,MEMFAULT_SDK_ROOT,\
  Must define the path to the root of the Memfault SDK)

MEMFAULT_COMPONENTS_DIR := $(MEMFAULT_SDK_ROOT)/components

MEMFAULT_COMPONENTS_INC_FOLDERS := $(MEMFAULT_COMPONENTS_DIR)/include

MEMFAULT_COMPONENTS_SRCS = \
  $(foreach component, $(MEMFAULT_COMPONENTS), \
    $(sort $(wildcard $(MEMFAULT_COMPONENTS_DIR)/$(component)/src/*.c)) \
  )

ifneq ($(filter demo,$(MEMFAULT_COMPONENTS)),)
# The demo component is enabled so let's pick up component specific cli commands
MEMFAULT_COMPONENTS_SRCS += \
  $(foreach component, $(MEMFAULT_COMPONENTS), \
    $(sort $(wildcard $(MEMFAULT_COMPONENTS_DIR)/demo/src/$(component)/*.c)) \
  )
endif

MEMFAULT_COMPONENTS_SRCS := \
  $(patsubst %memfault_fault_handling_xtensa.c, , $(MEMFAULT_COMPONENTS_SRCS))
