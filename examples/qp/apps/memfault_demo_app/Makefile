EMBEDDED_MFLT_SDK_ROOT ?=
QPC_DIR ?=
Q ?= @

ifeq ($(EMBEDDED_MFLT_SDK_ROOT),)
$(error Please set EMBEDDED_MFLT_SDK_ROOT environment variable to point to the where the Memfault Firmware SDK is installed.\
Download it from here: https://github.com/memfault/memfault-firmware-sdk/releases)
endif

ifeq ($(QPC_DIR),)
$(error Please set QPC_DIR environment variable to point to the where qpc/ is installed.\
Download it from here: https://github.com/QuantumLeaps/qp-bundle/releases)
endif

MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
SRC_DIR := $(dir $(MKFILE_PATH))src
CC = arm-none-eabi-gcc
LD = arm-none-eabi-ld
OCPY = arm-none-eabi-objcopy
ODUMP = arm-none-eabi-objdump
MKDIR ?= mkdir
RM ?= rm
BUILD_DIR := $(CURDIR)/build
PROJECT ?= memfault_demo_app

MEMFAULT_COMPONENTS := core panics util demo http
MEMFAULT_SDK_ROOT := $(EMBEDDED_MFLT_SDK_ROOT)
include $(MEMFAULT_SDK_ROOT)/makefiles/MemfaultWorker.mk

MEMFAULT_PORT_SRCS := \
	$(EMBEDDED_MFLT_SDK_ROOT)/ports/panics/src/memfault_platform_ram_backed_coredump.c

QF_SRC_DIR := $(QPC_DIR)/src/qf
QV_SRC_DIR := $(QPC_DIR)/src/qv
QP_SRCS := \
	$(QF_SRC_DIR)/qep_hsm.c \
	$(QF_SRC_DIR)/qep_msm.c \
	$(QF_SRC_DIR)/qf_act.c \
	$(QF_SRC_DIR)/qf_actq.c \
	$(QF_SRC_DIR)/qf_defer.c \
	$(QF_SRC_DIR)/qf_dyn.c \
	$(QF_SRC_DIR)/qf_mem.c \
	$(QF_SRC_DIR)/qf_ps.c \
	$(QF_SRC_DIR)/qf_qact.c \
	$(QF_SRC_DIR)/qf_qeq.c \
	$(QF_SRC_DIR)/qf_qmact.c \
	$(QF_SRC_DIR)/qf_time.c \
	$(QV_SRC_DIR)/qv.c \
	$(QPC_DIR)/ports/arm-cm/qv/gnu/qv_port.c

QP_INCLUDES := \
	$(QPC_DIR)/include \
	$(QPC_DIR)/src \
	$(QPC_DIR)/ports/arm-cm/qv/gnu \
	$(QPC_DIR)/3rd_party/CMSIS/Include \
	$(QPC_DIR)/3rd_party/stm32f4-discovery \
	$(QPC_DIR)/3rd_party/stm32f4-discovery/inc \


SRCS := \
	$(SRC_DIR)/bsp.c \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/platform_reference_impl/memfault_platform_core.c \
	$(SRC_DIR)/platform_reference_impl/memfault_platform_log.c \
	$(SRC_DIR)/startup_stm32f4xx.c \
	$(QPC_DIR)/3rd_party/stm32f4-discovery/system_stm32f4xx.c \
	$(QPC_DIR)/3rd_party/stm32f4-discovery/src/stm32f4xx_gpio.c \
	$(QPC_DIR)/3rd_party/stm32f4-discovery/src/stm32f4xx_rcc.c \
	$(QPC_DIR)/3rd_party/stm32f4-discovery/src/stm32f4xx_usart.c \
	$(QP_SRCS) \
	$(MEMFAULT_COMPONENTS_SRCS) \
	$(MEMFAULT_PORT_SRCS)

INCLUDES := \
	$(SRC_DIR) \
	$(SRC_DIR)/../config \
	$(MEMFAULT_COMPONENTS_INC_FOLDERS) \
	$(QP_INCLUDES)

CFLAGS += \
	-mcpu=cortex-m4 \
	-mthumb \
	-Wall \
	-Werror \
	-std=c11 \
	-O0 \
	-g3 \
	-ffreestanding \
	-ffunction-sections \
	-fdata-sections

# system_stm32f4xx.c trips a "misleading indentation" error:
CFLAGS += -Wno-misleading-indentation

LDFLAGS += \
	-specs=nosys.specs \
	-Wl,--build-id \
	-Wl,--gc-sections \
	-Wl,-Map=$(BUILD_DIR)/$(PROJECT).map \
	-T $(SRC_DIR)/arm-generic.ld

DEFINES +=
CFLAGS += $(foreach i,$(INCLUDES),-I$(i))
CFLAGS += $(foreach d,$(DEFINES),-D$(d))

.PHONY: all
all: $(BUILD_DIR)/$(PROJECT).elf

.PHONY : clean
clean:
	-$(RM) -rf $(BUILD_DIR)

$(BUILD_DIR)/$(PROJECT).elf: $(SRCS)
	@echo "Generating $@"
	@$(MKDIR) -p $(BUILD_DIR)
	$(Q) $(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@
